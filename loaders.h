/*********************************************************************
 * loaders.h
 *
 * common loader include file
 *
 *
 * (C) 2016 Elecraft Inc. paul.saffren.n6hz
 *
 ********************************************************************/

#ifndef LOADERS_H_

#define LOADERS_H_



// -------------------------------------------------------------------------
// Return codes for shell
// -------------------------------------------------------------------------
#define LOADER_SUCCESS			0
#define LOADER_DEVICE_ERROR		-1
#define LOADER_FILE_ERROR		-2
#define LOADER_BAD_ARGS			-3
#define LOADER_CHECKSUM_FAIL	-4
#define LOADER_ERASE_FAIL		-5
#define LOADER_WRITE_FAIL		-6
#define LOADER_COMMAND_ERROR	-7
#define LOADER_TIMEOUT_ERROR	-8




// -------------------------------------------------------------------------
// Common Structs & Types
// -------------------------------------------------------------------------


// union for manipulating pieces of a 32 bit value
typedef union uReg32
{
	unsigned long 	Val32;
	unsigned short	Val16[2];
	unsigned char 	Val8[4];

	struct
	{
		unsigned short LW;
		unsigned short HW;
	} word;
	struct
	{
		unsigned char	LB;
		unsigned char	HB;
		unsigned char	UB;
		unsigned char	MB;
	} byte;

} uReg32;





// Union for manipulating an unsigned char by bits
typedef union BitByte
{
	struct
	{
		unsigned  BIT0:1;
		unsigned  BIT1:1;
		unsigned  BIT2:1;
		unsigned  BIT3:1;
		unsigned  BIT4:1;
		unsigned  BIT5:1;
		unsigned  BIT6:1;
		unsigned  BIT7:1;
	}bits;

	unsigned char byt;

} BitByte;









#endif
/********************************* eof loaders.h **********************/


 
