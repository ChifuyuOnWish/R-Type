#!/bin/sh
# Formats every C++ file tracked by Git. Run from the repository root.
git ls-files '*.cpp' '*.hpp' '*.h' '*.cc' '*.cxx' '*.hxx' '*.inl' | xargs -r clang-format -i
