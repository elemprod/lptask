
# Build with the Default Configuration
all:
	cd ./projects/access_test && $(MAKE)
	cd ./projects/interval_test && $(MAKE)
	cd ./projects/interval_math && $(MAKE)
	cd ./projects/pool_test && $(MAKE)
	
# Build with task buffer clearing enabled.  (normally disabled) 
buff_clear_enable:

	cd ./projects/access_test && $(MAKE) CFLAGS='-DSCHED_TASK_BUFF_CLEAR_EN=1'
	cd ./projects/interval_test && $(MAKE) CFLAGS='-DSCHED_TASK_BUFF_CLEAR_EN=1'
	cd ./projects/interval_math && $(MAKE) CFLAGS='-DSCHED_TASK_BUFF_CLEAR_EN=1'	
	cd ./projects/pool_test && $(MAKE) CFLAGS='-DSCHED_TASK_BUFF_CLEAR_EN=1'

# Build with task pools disabled. (normally enabled)
task_pools_disable:

	cd ./projects/access_test && $(MAKE) CFLAGS='-DSCHED_TASK_POOL_EN=0'
	cd ./projects/interval_test && $(MAKE) CFLAGS='-DSCHED_TASK_POOL_EN=0'
	cd ./projects/interval_math && $(MAKE) CFLAGS='-DSCHED_TASK_POOL_EN=0'	
	cd ./projects/pool_test && $(MAKE) CFLAGS='-DSCHED_TASK_POOL_EN=0'

# Build with the next expiring task caching disabled. (normally enabled)
task_cache_disable:

	cd ./projects/access_test && $(MAKE) CFLAGS='-DSCHED_TASK_CACHE_EN=0'
	cd ./projects/interval_test && $(MAKE) CFLAGS='-DSCHED_TASK_CACHE_EN=0'
	cd ./projects/interval_math && $(MAKE) CFLAGS='-DSCHED_TASK_CACHE_EN=0'	
	cd ./projects/pool_test && $(MAKE) CFLAGS='-DSCHED_TASK_CACHE_EN=0'
	

clean:
	cd ./projects/access_test && $(MAKE) clean
	cd ./projects/interval_test && $(MAKE) clean
	cd ./projects/interval_math && $(MAKE) clean	
	cd ./projects/pool_test && $(MAKE) clean
			