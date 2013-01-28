# This file is not executable for a reason. It is a build library.
# See build-helper usage in readme.rst.
# Long story short: create directory for your project, create build.bash,
# srcs=( your.c files.c ), source build-helper.bash, main $*.

if ! declare -p srcs >/dev/null; then
	echo "build-helper: error: srcs not declared. Please, read readme.rst for usage." >&2
	exit 1
fi

# Location of OpenOCD Board .cfg files (only used with 'build.bash program')
: ${openocd_board_dir="/usr/share/openocd/scripts/board"}

# Location of the STM32F0xx Standard Peripheral Library
# Note: `build` target will run make in this directory
# if linked library is not found.
: ${stm32_periph_lib_dir="lib/STM32F0xx_StdPeriph_Driver"}

# that's it, no need to change anything below this line!

###################################################

project_path="$(dirname $0)"
if [ "$project_path" = "." ]; then
	project_path="$PWD"
else
	project_path="$PWD/$project_path"
fi

srcs_len=${#srcs[*]}
for (( i=0; i < $srcs_len; i++ )); do
	if [[ "${srcs[$i]}" != "/"* ]]; then
		srcs[$i]="${project_path}/${srcs[$i]}"
	fi
	if [ ! -r "${srcs[$i]}" ]; then
		echo "build-helper: error: source ${srcs[$i]} is declared but not found." >&2
		exit 2
	fi
done

srcs+=" lib/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.c "
srcs=${srcs[@]}

: ${CC=arm-none-eabi-gcc}
: ${LD=arm-none-eabi-ld}
: ${OBJCOPY=arm-none-eabi-objcopy}
: ${OBJDUMP=arm-none-eabi-objdump}
: ${SIZE=arm-none-eabi-size}
: ${GDB=arm-none-eabi-gdb}
: ${MAKE=make}

cflags=(
	-Wall -Werror -Os -g -std=c99 -ffreestanding -ffunction-sections -fdata-sections
	-Wl,--gc-sections -Wl,-Map="_build/${project}.map"
	-Wl,--start-group -lgcc -lc -lm -lnosys -Wl,--end-group
	-mlittle-endian -mcpu=cortex-m0 -march=armv6-m -mthumb
	# -mlittle-endian -mthumb -mcpu=cortex-m0 -march=armv6s-m
	-I lib/Device/ST/STM32F0xx/Include -I lib/CMSIS/Include
	-I ${stm32_periph_lib_dir} -I ${stm32_periph_lib_dir}/inc
)
CFLAGS="${CFLAGS} ${cflags[@]}"

linker_script="lib/Device/szczys/stm32f0.ld"
ldflags=(
	-L ${stm32_periph_lib_dir} -lstm32f0
	-L $(dirname "$linker_script") -T${linker_script}
)
LDFLAGS="${LDFLAGS} ${ldflags[@]}"

root=$PWD
binary_build="${root}/_build/${project}.bin"
elf_build="${root}/_build/${project}.elf"

# Actions

do_build() {
	# Build peripheral library
	if [ ! -r "$stm32_periph_lib_dir/libstm32f0.a" ]; then
		${MAKE} --quiet -j9 -C "$stm32_periph_lib_dir"
	fi

	# Compile
	mkdir -p _build
	for f in ${srcs//.c/}; do
		name=${f##*/}
		out=_build/$name.o
		# ${CC} ${CFLAGS} -o "$out" -c "$f.c"
		objs="$objs $out"
	done

	# Link to ELF
	# add startup file to build
	startup=lib/Device/szczys/startup_stm32f0xx.s
	${CC} ${CFLAGS} ${srcs} ${startup} ${LDFLAGS} -o "$elf_build"

	# Make binary
	${OBJCOPY} -O ihex "$elf_build" "_build/${project}.hex"
	${OBJCOPY} -O binary "$elf_build" "$binary_build"

	# Create listing
	${OBJDUMP} -St "$elf_build" >"_build/${project}.lst"
	${SIZE} "$elf_build"
}

do_clean() {
	find . -name '*~' -delete
	rm -f _build/*
	${MAKE} --quiet -C "$stm32_periph_lib_dir" clean
}

do_debug() {
	if [ ! -r "$elf_build" ]; then
		do_build
	fi

	st-util &
	local link_pid=$!
	if [ ! -r gdb-script ]; then
		{
			echo "target extended-remote localhost:4242"
			echo "break main.c:main"
		} >>gdb-script
	fi
	${GDB} "$elf_build" -command=gdb-script
	wait "$link_pid"
}

do_format() {
	astyle --indent=spaces=4 --brackets=break \
		--indent-labels --pad-oper --unpad-paren --pad-header \
		--keep-one-line-statements --convert-tabs \
		--indent-preprocessor \
		`find -type f -name '*.c' -or -name '*.cpp' -or -name '*.h'`
}

do_program() {
	if [ ! -r "$binary_build" ]; then
		do_build
	fi

	openocd -f "${openocd_board_dir}/stm32f0discovery.cfg" -f "lib/stm32f0-openocd.cfg" -c stm_erase -c "stm_flash ${binary_build}" -c shutdown
}

###################################################

main() {
	# Parse command line arguments
	if [ -z "$1" ]; then
		do_build
	fi

	while [ -n "$1" ]; do
		fun="do_${1}"
		if declare -f -F "$fun" >/dev/null; then
			$fun
		else
			echo "Unknown command: '$1'. Function is not defined." >&2
			exit 1
		fi
		shift
	done
}
