/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include "nir.h"
#include <main/imports.h>

/**
 * SSA-based copy propagation
 */

static bool is_move(nir_alu_instr *instr)
{
   if (instr->op != nir_op_fmov &&
       instr->op != nir_op_imov)
      return false;

   if (instr->dest.saturate)
      return false;

   /* we handle modifiers in a separate pass */

   if (instr->src[0].abs || instr->src[0].negate)
      return false;

   if (!instr->src[0].src.is_ssa)
      return false;

   return true;

}

static bool
is_swizzleless_move(nir_alu_instr *instr)
{
   if (!is_move(instr))
      return false;

   for (unsigned i = 0; i < 4; i++) {
      if (!((instr->dest.write_mask >> i) & 1))
         break;
      if (instr->src[0].swizzle[i] != i)
         return false;
   }

   return true;
}

static bool is_vec(nir_alu_instr *instr)
{
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++)
      if (!instr->src[i].src.is_ssa)
         return false;

   return instr->op == nir_op_vec2 ||
          instr->op == nir_op_vec3 ||
          instr->op == nir_op_vec4;
}

typedef struct {
   nir_ssa_def *def;
   bool found;
} search_def_state;

static bool
search_def(nir_src *src, void *_state)
{
   search_def_state *state = (search_def_state *) _state;

   if (src->is_ssa && src->ssa == state->def)
      state->found = true;

   return true;
}

static void
rewrite_src_instr(nir_src *src, nir_ssa_def *new_def, nir_instr *parent_instr)
{
   nir_ssa_def *old_def = src->ssa;

   src->ssa = new_def;

   /*
    * The instruction could still use the old definition in one of its other
    * sources, so only remove the instruction from the uses if there are no
    * more uses left.
    */

   search_def_state search_state;
   search_state.def = old_def;
   search_state.found = false;
   nir_foreach_src(parent_instr, search_def, &search_state);
   if (!search_state.found) {
      struct set_entry *entry =
         _mesa_set_search(old_def->uses, _mesa_hash_pointer(parent_instr),
                          parent_instr);
      assert(entry);
      _mesa_set_remove(old_def->uses, entry);
   }

   _mesa_set_add(new_def->uses, _mesa_hash_pointer(parent_instr),
                 parent_instr);
}

static void
rewrite_src_if(nir_if *if_stmt, nir_ssa_def *new_def)
{
   nir_ssa_def *old_def = if_stmt->condition.ssa;

   if_stmt->condition.ssa = new_def;

   struct set_entry *entry =
      _mesa_set_search(old_def->if_uses, _mesa_hash_pointer(if_stmt), if_stmt);
   assert(entry);
   _mesa_set_remove(old_def->if_uses, entry);

   _mesa_set_add(new_def->if_uses, _mesa_hash_pointer(if_stmt), if_stmt);
}

static bool
copy_prop_src(nir_src *src, nir_instr *parent_instr, nir_if *parent_if)
{
   if (!src->is_ssa) {
      if (src->reg.indirect)
         return copy_prop_src(src, parent_instr, parent_if);
      return false;
   }

   nir_instr *src_instr = src->ssa->parent_instr;
   if (src_instr->type != nir_instr_type_alu)
      return false;

   nir_alu_instr *alu_instr = nir_instr_as_alu(src_instr);
   if (!is_swizzleless_move(alu_instr))
      return false;

   if (parent_instr)
      rewrite_src_instr(src, alu_instr->src[0].src.ssa, parent_instr);
   else
      rewrite_src_if(parent_if, alu_instr->src[0].src.ssa);

   return true;
}

static bool
copy_prop_alu_src(nir_alu_instr *parent_alu_instr, unsigned index)
{
   nir_alu_src *src = &parent_alu_instr->src[index];
   if (!src->src.is_ssa) {
      if (src->src.reg.indirect)
         return copy_prop_src(src->src.reg.indirect, &parent_alu_instr->instr,
                              NULL);
      return false;
   }

   nir_instr *src_instr =  src->src.ssa->parent_instr;
   if (src_instr->type != nir_instr_type_alu)
      return false;

   nir_alu_instr *alu_instr = nir_instr_as_alu(src_instr);
   if (!is_move(alu_instr) && !is_vec(alu_instr))
      return false;

   nir_ssa_def *def;
   unsigned new_swizzle[4] = {0, 0, 0, 0};

   if (alu_instr->op == nir_op_fmov ||
       alu_instr->op == nir_op_imov) {
      for (unsigned i = 0; i < 4; i++)
         new_swizzle[i] = alu_instr->src[0].swizzle[src->swizzle[i]];
      def = alu_instr->src[0].src.ssa;
   } else {
      def = NULL;

      for (unsigned i = 0; i < 4; i++) {
         if (!nir_alu_instr_channel_used(parent_alu_instr, index, i))
            continue;

         nir_ssa_def *new_def = alu_instr->src[src->swizzle[i]].src.ssa;
         if (def == NULL)
            def = new_def;
         else {
            if (def != new_def)
               return false;
         }
         new_swizzle[i] = alu_instr->src[src->swizzle[i]].swizzle[0];
      }
   }

   for (unsigned i = 0; i < 4; i++)
      src->swizzle[i] = new_swizzle[i];

   rewrite_src_instr(&src->src, def, &parent_alu_instr->instr);

   return true;
}

typedef struct {
   nir_instr *parent_instr;
   bool progress;
} copy_prop_state;

static bool
copy_prop_src_cb(nir_src *src, void *_state)
{
   copy_prop_state *state = (copy_prop_state *) _state;
   while (copy_prop_src(src, state->parent_instr, NULL))
      state->progress = true;

   return true;
}

static bool
copy_prop_instr(nir_instr *instr)
{
   if (instr->type == nir_instr_type_alu) {
      nir_alu_instr *alu_instr = nir_instr_as_alu(instr);
      bool progress = false;

      for (unsigned i = 0; i < nir_op_infos[alu_instr->op].num_inputs; i++)
         while (copy_prop_alu_src(alu_instr, i))
            progress = true;

      if (alu_instr->has_predicate)
         while (copy_prop_src(&alu_instr->predicate, instr, NULL))
            progress = true;

      if (!alu_instr->dest.dest.is_ssa && alu_instr->dest.dest.reg.indirect)
         while (copy_prop_src(alu_instr->dest.dest.reg.indirect, instr, NULL))
            progress = true;

      return progress;
   }

   copy_prop_state state;
   state.parent_instr = instr;
   state.progress = false;
   nir_foreach_src(instr, copy_prop_src_cb, &state);

   return state.progress;
}

static bool
copy_prop_if(nir_if *if_stmt)
{
   return copy_prop_src(&if_stmt->condition, NULL, if_stmt);
}

static bool
copy_prop_block(nir_block *block, void *_state)
{
   bool *progress = (bool *) _state;

   nir_foreach_instr(block, instr) {
      if (copy_prop_instr(instr))
         *progress = true;
   }

   if (block->cf_node.node.next != NULL && /* check that we aren't the end node */
       !nir_cf_node_is_last(&block->cf_node) &&
       nir_cf_node_next(&block->cf_node)->type == nir_cf_node_if) {
      nir_if *if_stmt = nir_cf_node_as_if(nir_cf_node_next(&block->cf_node));
      if (copy_prop_if(if_stmt))
         *progress = true;
   }

   return true;
}

bool
nir_copy_prop_impl(nir_function_impl *impl)
{
   bool progress = false;

   nir_foreach_block(impl, copy_prop_block, &progress);
   return progress;
}

bool
nir_copy_prop(nir_shader *shader)
{
   bool progress = false;

   nir_foreach_overload(shader, overload) {
      if (overload->impl && nir_copy_prop_impl(overload->impl))
         progress = true;
   }

   return progress;
}