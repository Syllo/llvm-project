// REQUIRES: hexagon-registered-target || x86-registered-target
// RUN: split-file %s %t
// RUN: mkdir -p %t/target
// RUN: sed "s@TEST_DIR@%{/t:regex_replacement}@g" %t/overlay.yaml.in > %t/overlay.yaml

// The runtime include directories exist only in the frontend's VFS overlay.
// Exercise both overlay spellings and Hexagon's CPU/G0 path selection.
// RUN: %if hexagon-registered-target %{ %clang --target=hexagon -mv79 -G0 -fenable-ripple -B%t/target -nostdinc -ivfsoverlay %t/overlay.yaml -fsyntax-only %t/test.c %}
// RUN: %if hexagon-registered-target %{ %clang --target=hexagon -mv79 -G0 -fenable-ripple -B%t/target -nostdinc --vfsoverlay %t/overlay.yaml -fsyntax-only %t/test.c %}
// RUN: %if x86-registered-target %{ %clang --target=x86_64-unknown-linux-gnu -fenable-ripple -resource-dir %t/resource -nostdinc -ivfsoverlay %t/overlay.yaml -fsyntax-only %t/test.c %}
// RUN: %if x86-registered-target %{ %clang --target=x86_64-unknown-linux-gnu -fenable-ripple -resource-dir %t/resource -nostdinc --vfsoverlay %t/overlay.yaml -fsyntax-only %t/test.c %}

// These remain non-system headers, so -MMD must list the runtime header.
// RUN: %if hexagon-registered-target %{ %clang --target=hexagon -mv79 -G0 -fenable-ripple -B%t/target -nostdinc -ivfsoverlay %t/overlay.yaml -fsyntax-only -MMD -MF %t/hexagon.d %t/test.c && FileCheck %s --check-prefix=DEP < %t/hexagon.d %}
// RUN: %if x86-registered-target %{ %clang --target=x86_64-unknown-linux-gnu -fenable-ripple -resource-dir %t/resource -nostdinc -ivfsoverlay %t/overlay.yaml -fsyntax-only -MMD -MF %t/x86.d %t/test.c && FileCheck %s --check-prefix=DEP < %t/x86.d %}
// DEP: runtime.h

//--- test.c
#include <ripple-runtime.h>
int value = RIPPLE_RUNTIME_VALUE;

//--- runtime.h
#define RIPPLE_RUNTIME_VALUE 42

//--- overlay.yaml.in
{
  'version': 0,
  'roots': [
    { 'type': 'directory',
      'name': 'TEST_DIR/target/hexagon/lib/v79/G0/include',
      'contents': [
        { 'type': 'file', 'name': 'ripple-runtime.h',
          'external-contents': 'TEST_DIR/runtime.h' }
      ]
    },
    { 'type': 'directory',
      'name': 'TEST_DIR/resource/lib/x86_64-unknown-linux-gnu/ripple/include',
      'contents': [
        { 'type': 'file', 'name': 'ripple-runtime.h',
          'external-contents': 'TEST_DIR/runtime.h' }
      ]
    }
  ]
}
