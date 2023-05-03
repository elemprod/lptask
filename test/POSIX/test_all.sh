#!/usr/bin/env bash
trap 'exit' ERR

# Build the Test Projects
make clean
make all

clear

echo "*** Scheduler Library Test ***"
# Task Access Test
if ./projects/access_test/build/access_test; then
  echo "Access Test: Pass"
else
  echo "Access Test: FAIL"
fi

# Interval Math Test
if ./projects/interval_math/build/interval_math; then
  echo "Interval Math Test: Pass"
else
  echo "Interval Math Test: FAIL"
fi

# Pool Test
if ./projects/pool_test/build/pool_test; then
  echo "Task Pool Test: Pass"
else
  echo "Task Pool Test: FAIL"
fi

#TODO Make a shortened interval test which checks the interval error.

echo ""





