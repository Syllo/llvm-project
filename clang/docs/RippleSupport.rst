.. contents::
   :local:

==================================
Ripple Language Extensions Support
==================================

Clang has full support for the vectorization and loop annotation API exposed by Ripple.

TODO: add link to publicly available API reference manual.

Internals Manual
================

This section acts as internal documentation for Ripple features design
as well as some important implementation aspects. It is primarily targeted
at the advanced users and the toolchain developers integrating frontend
functionality as a component.

Ripple API Support
------------------

Clang exposes most of the Ripple API through internal headers:

- ``ripple.h``: Core API and pragma definitions.
- ``ripple_math.h``: Math builtins.
- ``ripple_hvx.h``: HVX-specific helpers.
- ``ripple/zip.h``: Tuple unzipping and block loading utilities.
- ``__ripple_vec.h``: High-level C/C++ interfaces for vectorization and builtins.

Ripple Builtins
---------------

Clang exposes the Ripple API as core (target-independent) builtins:

- Builtins are defined in ``BuiltinsRipple.td`` and integrated via ``Builtins.td``.
- These include:
  - Core: ``__builtin_ripple_get_index``, ``__builtin_ripple_get_size``, ``__builtin_ripple_set_shape``, ``__builtin_ripple_parallel_idx``.
  - Reductions: ``__builtin_ripple_reduceadd_*``, ``__builtin_ripple_reducemax_*``, ``__builtin_ripple_reducemin_*``, ``__builtin_ripple_reduceand_*``, ``__builtin_ripple_reduceor_*``.
  - Math: ``__builtin_ripple_sqrt``, ``__builtin_ripple_exp``, ``__builtin_ripple_log``, etc.
  - Shuffle/Broadcast: ``__builtin_ripple_shuffle_*``, ``__builtin_ripple_broadcast_*``, ``__builtin_ripple_slice_*``.

Native Vector Conversions
-------------------------

``__builtin_ripple_from_fixed_vector(block, vector)`` returns a scalar with the
vector's element type. The result is a Ripple value whose shape is the shape of
the supplied block.
``__builtin_ripple_to_fixed_vector(count, scalar)`` returns a native vector of
``count`` elements of the scalar's type, except that a count of one returns the
scalar type directly. The count must be a positive integer constant no greater
than half the compiler's maximum ``int`` value (``INT_MAX / 2``); it determines
the return type and is not an LLVM intrinsic operand. The Ripple value's
propagated shape determines the source shape.

The ``vec_to_ripple`` and ``ripple_to_vec`` helpers use these builtins. When a
function is processed with an active Ripple shape, the conversions operate on
the complete resolved block. The native-vector value and Ripple value must have
equal total bit width. Ripple diagnoses unequal widths after shape propagation,
and neither fixed-vector value is transformed by Ripple. The pass lowers both
conversions without temporary arrays.

``vec_to_ripple(block, vector)`` infers both the element type and element count
from the vector argument. Legacy untyped vectors can explicitly select an
element type with ``vec_to_ripple<type>(block, vector)`` in C++ or
``vec_to_ripple(type, block, vector)`` in C. In C++,
``ripple_to_vec<count>(value)`` can infer the element type from ``value``. In C,
the equivalent spelling is ``ripple_to_vec(count, value)``. Both languages also
provide an explicit element-type form: ``ripple_to_vec<count, type>(value)`` in
C++ and ``ripple_to_vec(count, type, value)`` in C. An explicit type may be
needed when integer promotions or a legacy target API obscure the operation's
semantic element type.

Passing a block shape as a function argument is intended only to request
specialization. The callee's definition must be available in the same LLVM
module as the call so Ripple can specialize it using the concrete block shape.
The unspecialized function remains a scalar fallback, where ``vec_to_ripple``
selects lane zero and ``ripple_to_vec`` broadcasts the scalar. The C++ helpers
remain templates.

Ripple Semantics Analysis
-------------------------

Ripple constructs are semantically analyzed via the ``SemaRipple`` class:

- Validates canonical loop form for Ripple parallel constructs.
- Ensures dimension indices are unique.
- Checks builtin argument types and values.
- Constructs ``RippleComputeConstruct`` AST nodes for transformed loops.

Ripple AST Integration
----------------------

Ripple introduces a new AST node ``RippleComputeConstruct`` defined in ``StmtRipple.h``:

- Represents a parallel loop construct with full and remainder loop sections.
- Includes source ranges, block shape reference, dimension indices, and sub-statements.
- Supports traversal via ``RecursiveASTVisitor`` and printing via ``StmtPrinter``.

Ripple Builtin Infrastructure
-----------------------------

Ripple builtins are defined in ``BuiltinsRipple.td`` using TableGen templates:

- Integer and floating-point types are supported via type templates.
- Builtins are lowered to LLVM intrinsics in ``CGStmtRipple.cpp``.
- Semantic checks are performed in ``SemaRipple::CheckBuiltinFunctionCall``.

Ripple Code Generation
----------------------

Ripple constructs are lowered to LLVM IR via ``EmitRippleComputeConstruct``:

- Emits full iteration loop over blocks.
- Emits remainder loop if needed.
- Generates runtime condition and loop variable updates.
- Maps builtins to LLVM intrinsics.

Ripple Driver and Language Options
----------------------------------

Ripple support is enabled via:

- ``-fenable-ripple``: Enables Ripple transformation.

Ripple Diagnostics
------------------

Ripple introduces new diagnostics:

- ``warn_pragma_ripple_ignored``: Unexpected `#pragma ripple`.
- ``err_ripple_loop_not_for_loop``: Ripple only applies to `for` loops.
- ``err_ripple_loop_not_canonical_*``: Validates loop initializer, condition, increment.
- ``err_ripple_duplicate_parallel_index``: Duplicate dimension index.
- ``err_ripple_stmt_depends_on_loop_counter``: Loop expressions depend on control variable.

Defined in ``DiagnosticParseKinds.td`` and ``DiagnosticSemaKinds.td``.

Ripple Header Files
-------------------

Ripple headers are installed via ``lib/Headers/CMakeLists.txt``:

- ``ripple.h``, ``ripple_math.h``, ``ripple_hvx.h``, ``__ripple_vec.h``, ``ripple/zip.h``.
- Provide high-level interfaces for vectorization, math, and parallel constructs.
