#!/usr/bin/env bash
trap 'exit' ERR

RED='\033[0;31m'
NOCOLOR='\033[0m'

# Run the Each of the Tests.
access_test() {
  if ./projects/access_test/build/access_test; then
    echo "Task Access Test ($1): Pass"
  else
    printf "Task Acccess Test ($1): ${RED}FAIL${NOCOLOR}\n"
  fi
}
echo -e "\e[31mHello World\e[0m"

interval_math_test() {
  # Interval Math Test
  if ./projects/interval_math/build/interval_math; then
    echo "Interval Math Test ($1): Pass"
  else
    printf "Interval Math Test ($1): ${RED}FAIL${NOCOLOR}\n"
  fi

}

task_pool_test() {
  # Task Pool Test
  if ./projects/pool_test/build/pool_test; then
    echo "Task Pool Test ($1): Pass"
  else
    printf "Task Pool Test ($1): ${RED}FAIL${NOCOLOR}\n"
  fi  
}

clear

echo "*** Scheduler Library Test ***"

# Test with the Default Configuration
make clean
make all
access_test 'Default'
interval_math_test 'Default'
task_pool_test 'Default'

# Test the Buffer Clear Enabled Configuration
make clean
make buff_clear_enable
access_test 'Buff Clear Enabled'
interval_math_test 'Buff Clear Enabled'
task_pool_test 'Buff Clear Enabled'


# Test the Task Pool Disabled Configuration
make clean
make task_pools_disable
access_test 'Task Pools Disabled'
interval_math_test 'Task Pools Disabled'
# The task pool test would fail, skip test.
task_pool_test 'Task Pools Disabled'

# Test the Task Cache Disabled Configuration
make clean
make task_cache_disable
access_test 'Task Cache Disabled'
interval_math_test 'Task Cache Disabled'
task_pool_test 'Task Cache'

#TODO Make a shortened interval test and add it back in.

echo ""





