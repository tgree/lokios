#ifndef __KERNEL_RB_TREE_H
#define __KERNEL_RB_TREE_H

#include "kernel/kassert.h"

namespace kernel
{
    struct rbtree_node
    {
        static const rbtree_node leaf;

        rbtree_node*    parent = NULL;
        rbtree_node*    left   = NULL;
        rbtree_node*    right  = NULL;

        constexpr rbtree_node* grandparent() const
        {
            return parent ? parent->parent : NULL;
        }

        constexpr rbtree_node* sibling() const
        {
            auto* p = parent;
            return p == NULL       ? NULL :
                   p->left == this ? p->right :
                                     p->left;
        }

        constexpr rbtree_node* uncle() const
        {
            auto* p = parent;
            return p == NULL ? NULL : p->sibling();
        }

        void rotate_left()
        {
            rbtree_node* nnew = right;
            rbtree_node* p    = parent;
            kassert(nnew != leaf);
            right      = nnew->left;
            nnew->left = this;
            parent     = nnew;

            if (right)
                right->parent = this;
            if (p)
            {
                if (p->left == this)
                    p->left = nnew;
                else
                    p->right = nnew;
                nnew->parent = p;
            }
        }

        void rotate_right()
        {
            rbtree_node* nnew = left;
            rbtree_node* p    = parent;
            kassert(nnew != leaf);
            left        = nnew->right;
            nnew->right = this;
            parent      = nnew;

            if (left)
                left->parent = this;
            if (p)
            {
                if (p->left == this)
                    p->left = nnew;
                else
                    p->right = nnew;
                nnew->parent = p;
            }
        }
    };
}

#endif /* __KERNEL_RB_TREE_H */
