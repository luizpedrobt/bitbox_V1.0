#include <stdint.h>
#include <stddef.h>
#include <assert.h>

// ref: https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing

/** COBS encode data to buffer
	@param input Pointer to input data to encode
	@param output Pointer to encoded output buffer
	@param len Number of bytes to encode
	@return Encoded output len in bytes
	@note Does not output delimiter byte
*/
size_t utl_cobs_encode(const void *input, uint8_t *output, size_t len)
{
	assert(input && output);

	uint8_t *encode = output; // Encoded byte pointer
	uint8_t *codep = encode++; // Output code pointer
	uint8_t code = 1; // Code value

	for (const uint8_t *byte = (const uint8_t *)input; len--; ++byte)
	{
		if (*byte) // Byte not zero, write it
			*encode++ = *byte, ++code;

		if (!*byte || code == 0xff) // Input is zero or block completed, restart
		{
			*codep = code, code = 1, codep = encode;
			if (!*byte || len)
				++encode;
		}
	}
	*codep = code; // Write final code value

	return (size_t)(encode - output);
}

/** COBS decode data from buffer
	@param input Pointer to encoded input bytes
	@param len Number of bytes to decode
	@param output Pointer to decoded output data
	@return Number of bytes successfully decoded
	@note Stops decoding if delimiter byte is found
*/
size_t utl_cobs_decode(const uint8_t *input, void *output, size_t len)
{
	assert(input && output);

	const uint8_t *byte = input; // Encoded input byte pointer
	uint8_t *decode = (uint8_t *)output; // Decoded output byte pointer

	for (uint8_t code = 0xff, block = 0; byte < input + len; --block)
	{
		if (block) // Decode block byte
			*decode++ = *byte++;
		else
		{
			if (code != 0xff) // Encoded zero, write it
				*decode++ = 0;
			block = code = *byte++; // Next block len
			if (!code) // Delimiter code found
				break;
		}
	}

	return (size_t)(decode - (uint8_t *)output);
}