#!/bin/sh
# Not needed if you configure with CMake (it does this for you).
git config core.hooksPath .githooks && echo "Git hooks enabled (.githooks)."
