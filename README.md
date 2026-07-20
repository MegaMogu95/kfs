docker build -t kfs-toolchain .                  		# build image (first time / after Dockerfile edits)
docker run --rm -v "$PWD":/kfs kfs-toolchain    	 	# runs the CMD (make)
docker run --rm -v "$PWD":/kfs kfs-toolchain make re
docker run --rm -it -v "$PWD":/kfs kfs-toolchain bash   # interactive shell

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

run-kernel: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)