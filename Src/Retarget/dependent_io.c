#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "stdio.h"
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2

/**
    define device io data
*/

extern ARM_DRIVER_USART Driver_USART1;

osThreadId_t __sys_stdin_manager;
osThreadId_t __sys_stdout_manager;
osEventFlagsId_t __sys_stdio_flags;

#define BUFFER_SIZE 128
static uint8_t __DEVICE_IN_BUFFER[BUFFER_SIZE];
static uint8_t __DEVICE_OUT_BUFFER[BUFFER_SIZE];

struct __io_buffer { /*

    struct __io_buffer is a cyclic queue used to store buffer device input/output
    
    Fields:
        buffer : point to the begin address of buffer
        size   : indicate the size of buffer, it may not be the actual size of buffer
        pos    : start point of stream
        end    : end point of stream
        flags  : status of buffer
    
*/
    uint8_t* buffer;
    size_t size;
    size_t pos;
    size_t end;
    uint32_t flags;
};
static struct __io_buffer __in_buffer = {__DEVICE_IN_BUFFER, BUFFER_SIZE, 0, 0, 0x0};
static struct __io_buffer __out_buffer = {__DEVICE_OUT_BUFFER, BUFFER_SIZE, 0, 0, 0x0};


/**
    define device io informations flags
*/

#define RECEIVE_COMPLETE      0x00000001U
#define SEND_COMPLETE         0x00000002U
#define OUT_BUFFER_UPDATA     0x00000004U

#define BUFFER_ERROR          0x80000000U
#define BUFFER_ERROR_OVERFLOW (BUFFER_ERROR | 0x00000001U)


/**
    define device io methods
*/

static void __device_stdio_callback(uint32_t event) {
    switch (event) {
        case ARM_USART_EVENT_RECEIVE_COMPLETE:
            osEventFlagsSet(__sys_stdio_flags, RECEIVE_COMPLETE);
            break;
        case ARM_USART_EVENT_SEND_COMPLETE:
            osEventFlagsSet(__sys_stdio_flags, SEND_COMPLETE);
            break;
        default:
            break;
    }
}

static void __device_stdio_in(void* arg) { /*

    System device input service
    
    Wait device input, went receive any character, put it into __in_buffer
    When __in_buffer is full, set BUFFER_OVERFLOW flag and drop all follow-up characters
    untill __in_buffer reset
    
*/
    uint8_t in;
    for (;;) {
        if (__in_buffer.flags | BUFFER_ERROR) continue;
        
        Driver_USART1.Receive(&in, 1);
        osEventFlagsWait(__sys_stdio_flags, RECEIVE_COMPLETE, osFlagsWaitAll, osWaitForever);
        
        __in_buffer.buffer[__in_buffer.end] = in;
        __in_buffer.end = (__in_buffer.end + 1) % __in_buffer.size;
        
        if (__in_buffer.end == __in_buffer.pos) __in_buffer.flags |= BUFFER_ERROR_OVERFLOW;
    }
}

static void __device_stdio_out(void* arg) { /*

    System device output service
    
    send characters in buffer into device
    
*/
    for (;;) {
        if (__out_buffer.flags | BUFFER_ERROR) continue;
        if (__out_buffer.pos != __out_buffer.end) {
            Driver_USART1.Send(__out_buffer.buffer + __out_buffer.pos, 1);
            osEventFlagsWait(__sys_stdio_flags, SEND_COMPLETE, osFlagsWaitAll, osWaitForever);
            __out_buffer.pos = (__out_buffer.pos + 1) % __out_buffer.size;
        }
    }
}

int32_t device_stdio_init(void) { /*
    
    Initialize stdout, stdin, stderr
    
    use usart 1
    mode         : asynchronous
    band rate    : 115200
    data bits    : 8
    stop bits    : 1
    parity       : None
    flow control : None
    
*/
    extern ARM_DRIVER_USART Driver_USART1;
    uint32_t mode = ARM_USART_MODE_ASYNCHRONOUS;
    uint32_t bandrate = 115200;
    
    int32_t status;
    
    status = Driver_USART1.Initialize(__device_stdio_callback);
    if (ARM_DRIVER_OK != status) return status;
    
    status = Driver_USART1.PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status) return status;
    
    status = Driver_USART1.Control(mode, bandrate);
    if (ARM_DRIVER_OK != status) return status;
    
    status = Driver_USART1.Control(ARM_USART_CONTROL_RX, 1);
    if (ARM_DRIVER_OK != status) goto cleanup;
    
    status = Driver_USART1.Control(ARM_USART_CONTROL_TX, 1);
    if (ARM_DRIVER_OK != status) goto cleanup;
    
    return ARM_DRIVER_OK;
    
    cleanup:
    
    Driver_USART1.PowerControl(ARM_POWER_OFF);
    Driver_USART1.Uninitialize();
    return status;
}

int32_t sys_stdio_manage_init(void) { /*
    
    Initlialize IO events and IO manage thread
    
*/  
    __sys_stdio_flags = osEventFlagsNew(NULL);
    if (!__sys_stdio_flags) {
        return osErrorResource;
    }
    
    __sys_stdin_manager = osThreadNew(__device_stdio_in, NULL, NULL);
    if (!__sys_stdin_manager) {
        return osErrorResource;
    }
    
    __sys_stdout_manager = osThreadNew(__device_stdio_out, NULL, NULL);
    if (!__sys_stdout_manager) {
        return osErrorResource;
    }
    
    return osOK;
}


/**
    Implement low-level IO methods
*/

struct __FILE { /*
    
    manage file-like object
    
*/
    uint32_t handle;
    uint8_t* buffer;       // pointer to stream buffer
    size_t buffer_size;
    size_t pos;
    uint32_t flags;
};

/*
define std input/output
0 : stdin
1 : stdout
2 : stderr
*/
#define __STD_IN_HANDLE 0
#define __STD_OUT_HANDLE 1
#define __STD_ERR_HANDLE 2

struct __FILE __stdin = {__STD_IN_HANDLE, NULL, 0, 0, 0x0};
struct __FILE __stdout = {__STD_OUT_HANDLE, NULL, 0, 0, 0x0};
struct __FILE __stderr = {__STD_ERR_HANDLE, NULL, 0, 0, 0x0};

int stdout_putchar(int ch) { /*

    manage library low-level std output
    
    put character into output buffer
    
    Return:
        ch : everything going well
        negative number : error when work on buffer
*/
    if (__out_buffer.flags | BUFFER_ERROR) {
        return __out_buffer.flags;
    }
    
    __out_buffer.buffer[__out_buffer.end] = (uint8_t)ch;
    __out_buffer.end = (__out_buffer.end + 1) % __out_buffer.size;
    if (__out_buffer.end == __out_buffer.pos) {
        __out_buffer.flags |= BUFFER_ERROR_OVERFLOW;
    }
    
    return ch;
}

int stderr_putchar(int ch) { /*

    manage library low-level std error
    
    put characters direct into device, no buffer
    
*/
    uint8_t out = (uint8_t)ch;
    Driver_USART1.Send(&out, 1);
    osEventFlagsWait(__sys_stdio_flags, SEND_COMPLETE, osFlagsWaitAll, osWaitForever);
    return ch;
}

int stdin_getchar(void) { /*

    manage library low-level std input
    
    read a character from __in_buffer
    
    Return:
        ch : character readed in
        negative value : error code about buffer status
    
*/
    uint8_t in;
    
    if (__in_buffer.flags | BUFFER_ERROR) {
        return __in_buffer.flags;
    }
    
    if (__in_buffer.pos == __in_buffer.end) {
        return EOF;
    }
    
    in = __in_buffer.buffer[__in_buffer.pos];
    __in_buffer.pos = (__in_buffer.pos + 1) % __in_buffer.size;
    
    return (int)(in);
}

int fputc(int ch, FILE* f) { /*

    implement fputc
    
    [in] ch : char want print
    [in] f  : stream will print char

*/
    if (f->handle == __STD_IN_HANDLE) {
        return (-1);  // occur an error because specify stdin as input
    }
    
    if (f->handle == __STD_OUT_HANDLE) return (stdout_putchar(ch));
    if (f->handle == __STD_ERR_HANDLE) return (stderr_putchar(ch));
    
    return ch;
}

int ferror(FILE* f) { /*
    
    check if any error occured in flie stream
    
*/
    return 0;
}

int fgetc(FILE* f) { /*

    implement fgetc to use high-level function
    
*/
    if (f->handle == __STD_OUT_HANDLE || f->handle == __STD_ERR_HANDLE) {
        return (-1);
    }
    
    if (f->handle == __STD_IN_HANDLE) return (stdin_getchar());
    return (0);
}

int __backspace(FILE *stream) { /*
    
    __backspace must only be called after reading a character from the stream.
    You must not call it after a write, a seek, or immediately after opening the file.
    
    It returns to the stream the last character that was read from the stream,
    so that the same character can be read from the stream again by the next read operation.
    This means that a character that was read from the stream by scanf but that is not required
    is read correctly by the next function that reads from the stream.
    
    The value return ed by __backsapce is either 0(success) or EOF(failure).
    It returns EOF only if used incorrectly.
*/
    
    return (0);
}
