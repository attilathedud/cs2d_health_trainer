/*!
*   An offline health trainer for OSX CS2D 1.0.0.3.
*
*   Given a pid in argv[ 1 ] and an integer value in argv[ 2 ], set the health of the current
*   player to argv[ 2 ]. Health for the current player is stored at dword ptr [0x3136e0] + 0x164 as an
*   integer.
*/
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>

int main( int argc, char** argv )
{
    kern_return_t kern_return = 0;
    mach_port_t task = 0;
    long int pid = 0;
    char *endptr = NULL;
    
    uint32_t size = 0;
    pointer_t buffer_pointer = 0;
    unsigned char buffer[ 4 ] = { 0 };
    unsigned int address = 0;
    uint32_t value = 0;

    if( argc < 3 ) 
    {
        return 1;
    }

    pid = strtol( argv[ 1 ], &endptr, 10 );
    value = strtol( argv[ 2 ], &endptr, 10 );

    // Get a handle for CS2D
    kern_return = task_for_pid( mach_task_self(), pid, &task );
    if( kern_return != KERN_SUCCESS ) 
    {
        printf( "task_for_pid failed: %s\n", mach_error_string( kern_return ) );
        return 1;
    }

    // Read the player structure location
    kern_return = vm_read( task, 0x3136e0, sizeof( int ), &buffer_pointer, &size );
    if( kern_return != KERN_SUCCESS )
    {
        printf( "vm_read failed: %s\n", mach_error_string( kern_return ) );
        return 1;
    }

    // Cast down to a buffer and back to a unsigned int to deal with endianness
    memcpy( buffer, (const void*)buffer_pointer, size );
    memcpy( &address, buffer, sizeof( address ) );

    // Add the health offset
    address += 0x164;

    printf("Health address: 0x%x\n", address);

    // Write the provided value back
    kern_return = vm_write( task, address, (pointer_t)( unsigned char * )&value, sizeof( uint32_t ) );
    if( kern_return != KERN_SUCCESS )
    {
        printf( "vm_write failed: %s\n", mach_error_string( kern_return ) );
        return 1;
    }

    return 0;
}
