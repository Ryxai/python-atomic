python-atomic
=============

Integer and reference types supporting atomic operations. This extension uses
the [GCC atomic builtins](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html),
which isn't strictly necessary because the GIL serializes everything anyways.

Minor addition from the python-atomic built by Osandov, included a markable reference extension
