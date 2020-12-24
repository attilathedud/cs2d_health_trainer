# CS2D Health Trainer
An offline health trainer for OSX CS2D 1.0.0.3.

## Overview
This code builds off the task_for_pid code discussed [here.](https://github.com/attilathedud/macos_task_for_pid)

The prototype for vm_read:
```
kern_return_t vm_read( 
    vm_task_t target_task,
    vm_address_t address,
    vm_size_t size,
    size data_out,
    target_task data_count);
```

In practice, this tends to look like:
```
uint32_t size = 0;
pointer_t buffer_pointer = 0;

kern_return = vm_read( task, 0xDEADBEEF, sizeof( int ), &buffer_pointer, &size );
```

## Downcasting
One of the quirks of the mach operations is you can't directly read into a variable type you want (or at least, I haven't found a way). Looking at the [documentation:](http://web.mit.edu/darwin/src/modules/xnu/osfmk/man/vm_read.html)
```
data_out: Out-pointer to dynamic array of bytes returned by the read.
```

Since we have a pointer to an array, the onus is on us to downcast it to a type we want. The best way I've found is to always downcast to a character array and then cast to whatever type you want. This has two benefits:
1. Character arrays are incredibly versatile. Having data in one basically ensures you can modify it however you want.
2. Casting from a character array to a dword or qword solves any endianness issues you may have.

Examples for both these points:
1. In [mem_scan](https://github.com/attilathedud/mem_scan/blob/504af7f09d939f5a3b79b53476f2e0d9015f6a3d/mem_scan/src/mem_functions.c#L81), the results of a read region are stored in a large character array. This allows us to avoid the bottleneck of constantly reading from a processes' memory and also allows us to easily cast values up to an int on dword boundaries. 
2. In CS2D, pointers are stored in little-endian due to the architecture. By downcasting to a character array and then again to an integer, we change the encoding to big-endian, which is what we need to use in vm_write.

To downcast, you can use `memcpy`:
```
unsigned char buffer[ 4 ] = { 0 };
    
memcpy( buffer, (const void*)buffer_pointer, size );
```

This will place the exact contents at the memory address into buffer. For example:
```
//0xDEADBEEF 00 00 00 64

buffer[ 0 ] = 00;
buffer[ 1 ] = 00;
buffer[ 2 ] = 00;
buffer[ 3 ] = 0x64;
```

If we know 0xDEADBEEF is an integer type:
```
uint32_t num = 0;

memcpy( &num, buffer, sizeof( uint32_t ) );

// num now equals 100 (0x64)
```

## CS2D
The memory at 0x3136e0 is a pointer to the player structure. It looks something like:
```
    0x3136e0 00 2a 55 1c
```

Where the address of our player structure is dynamically allocated (in this example, it would be at 0x1c552a00). To get this value, we downcast to our character array and then cast again to an unsigned int:
```c
unsigned char buffer[ 4 ] = { 0 };
unsigned int address = 0;

memcpy( buffer, (const void*)buffer_pointer, size );
memcpy( &address, buffer, sizeof( address ) );

// address now equals 0x1c552a00
```

To get the health address, we can add an offset directly to the address:
```
address += 0x164;
```

## Writing
```
kern_return_t vm_write(
    vm_task_t target_task,
    vm_address_t address,
    pointer_t data,
    mach_msg_type_number_t data_count);
```

Writing is a lot more straight-forward. The only "gotcha" is that data needs to be a pointer to the array of bytes you wish to write. An example:
```
uint32_t value = 0;    //provided in argv[ 2 ]

kern_return = vm_write( task, address, (pointer_t)( unsigned char * )&value, sizeof( uint32_t ) );
```

If your write target lives in code, you will have to give that address the write protection flag (generally code segments only have the execute flag set). You can do this using vm_protect:
```c
vm_protect( task, address, sizeof( uint32_t ), 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE );
```

## Compiling
For the kernel to allow us to use task_for_pid, we need to give our application the SecTaskAccess permission. To do this, create a file called Info.plist with the following content:
```
<?xml version="1.0" encoding="UTF-8"?>  
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">  
    <plist version="1.0">  
    <dict>  
        <key>SecTaskAccess</key>
        <array>
            <string>allowed</string>
        </array>
    </dict>  
    </plist>  
```

When compiling, use `-sectcreate` to create a section for the plist: 
```
gcc hp_hack.c -sectcreate __TEXT __info_plist ./Info.plist -o hp_hack_o  
```

## Usage
Execute CS2D, find its process id with `ps`, and then run `sudo ./hp_hack_o _some_pid _some_hp_value`
