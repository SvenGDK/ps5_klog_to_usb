// Required header
#include <ps5/payload_main.h>
#include <ps5/libkernel.h>
#include <ps5/kernel.h>

#include <fcntl.h>

// This payload is firmware dependent because fork() must be resolved manually.
#if PS5_FW_VERSION == 0x403
#define OFFSET_FORK_FROM_READ 0x1DE0
#else
#define OFFSET_FORK_FROM_READ 0x0
#warning klog_to_usb does not support this firmware, must be updated to be able to find fork().
#endif

// Ran in child process
void run_kernel_log()
{
	int out_fds[1];
	int fd, ret, waitedfor;
	char temp[0x2000];

	// Set sleep amount
	struct timespec ts;
	ts.tv_nsec = 100000000;
	ts.tv_sec = 0;
	
	waitedfor = 0;
	
    // Create output directory on USB
    mkdir("/mnt/usb0/PS5-klog", 0777);
	
	// Open /dev/klog & create klog.txt output file
	fd = _open("/dev/klog", 0, 0);
    out_fds[0] = _open("/mnt/usb0/PS5-klog/klog.txt", O_WRONLY | O_CREAT, 0644);

	// Cannot open klog ...
	if(fd == -1) {
		_write(out_fds[0], "Cannot open /dev/klog\n", 23);
		exit(1);
	}
	
    if (out_fds[0] < 0) {
		_write(out_fds[0], "/mnt/usb0/PS5-klog/klog.txt\n", 23);
		exit(1);
    }

	_write(out_fds[0], "Successfully opened klog!\n", 10);

	for(;;)
	{
		// Sleep to save CPU usage
		_nanosleep(&ts, 0);
		waitedfor++;
		
		// Stop reading after 10 minutes (10 min = 600,000,000,000 ns)
		long long int convertedValue = waitedfor;
		if (convertedValue == 600000000000)
		{
			_close(fd);
			_close(out_fds[0]);
			break;
		}
		
		ret = _read(fd, temp, 0xFFF);
		if (ret > 0)
		{
			temp[ret + 1] = 0x00;
			
			if(_write(out_fds[0], temp, ret + 1) != (ret + 1))
			{
				_close(fd);
				_close(out_fds[0]);
				break;
			}
		}
	}
}

extern void *fptr__read;

int payload_main(struct payload_args *args)
{
	int mainPID;
	
	// Fork must be resolved manually, dlsym refuses to resolve it
	void (*fptr_fork)() = (void (*)())(fptr__read + OFFSET_FORK_FROM_READ);

	// Fork so we can keep a klog server outside the browser process
	mainPID = getpid();
	fptr_fork();

	if (getpid() != mainPID)
	{
		run_kernel_log();
	}

	return 0;
}