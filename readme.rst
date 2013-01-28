What
====

This is my toy project to get familiar with STM32F0-Discovery board
and wonderful world of ARM Cortex-M processors.

Based on template https://github.com/szczys/stm32f0-discovery-basic-template

I already learned that coding tick-by-tick style is very tedious, so
some kind of task management is required. One way is to go with some
existing embedded OS, like ChibiOS, FreeRTOS, RTX, ucLinux, etc. But
first i'm going to try a simplest possible custom solution and see
how far it can get. Fun!


Build
=====

build-helper.bash is a very imperative custom build system.
Here's how to use it::

	mkdir new-project
	cat >new-project/build.bash <<EOF
	#!/bin/bash
	set -e
	cd $(dirname $0) ; cd ..
	srcs=(
	  main.c
	)
	source build-helper.bash
	main $*
	EOF

	chmod +x new-project/build.bash
	new-project/build.bash clean build program


Roadmap
=======

* Context switch code
* Critical section
* Create task, direct switch to it - cooperative
* Scheduler, lists of active/waiting tasks
* Call scheduler in timer - preemptive
* Sleep to unschedule current task - this should be enough
* Event - 1:1 synchronization primitive - advanced level
* Custom OS is too hard, let's take existing
* Add camera, servos with guns and blades
* ...
* Fully autonomous killer robot
* Another robot goes time travel to make me stop at the Sleep step.


Thanks
======

Thanks to https://github.com/szczys everything but his template did not work for me.

Thanks to Sherief Farouk for https://bitbucket.org/sherief/binary-to-c-array
Shortcut::

    curl -LO https://bitbucket.org/sherief/binary-to-c-array/raw/tip/source/binary-to-c-array.cpp
    g++ -o b2c -Wall binary-to-c-array.cpp
