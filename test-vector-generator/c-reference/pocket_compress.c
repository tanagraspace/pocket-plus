/*
 ============================================================================
 Name        : pocket_compresss.c
 Author      : Jonathan Nussbickel, ESA/ESOC
 Version     : 3.0
 Copyright   : No copyright
 Description : POCKET+ Compression in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////
/* --- PRINTF_BYTE_TO_BINARY macro's for debugging only --- */
//////////////////////////////////////////////////////////////
// #define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
// #define PRINTF_BYTE_TO_BINARY_INT8(i)    \
//     (((i) & 0x80ll) ? '1' : '0'), \
//     (((i) & 0x40ll) ? '1' : '0'), \
//     (((i) & 0x20ll) ? '1' : '0'), \
//     (((i) & 0x10ll) ? '1' : '0'), \
//     (((i) & 0x08ll) ? '1' : '0'), \
//     (((i) & 0x04ll) ? '1' : '0'), \
//     (((i) & 0x02ll) ? '1' : '0'), \
//     (((i) & 0x01ll) ? '1' : '0')

// #define PRINTF_BINARY_PATTERN_INT16 \
//     PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
// #define PRINTF_BYTE_TO_BINARY_INT16(i) \
//     PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
// #define PRINTF_BINARY_PATTERN_INT32 \
//     PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
// #define PRINTF_BYTE_TO_BINARY_INT32(i) \
//     PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
// #define PRINTF_BINARY_PATTERN_INT64    \
//     PRINTF_BINARY_PATTERN_INT32             PRINTF_BINARY_PATTERN_INT32
// #define PRINTF_BYTE_TO_BINARY_INT64(i) \
//     PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
// /* --- end macros --- */


/////////////////////////////////////////
// GLOBAL VARIABLES AND INITIALISATION //
/////////////////////////////////////////

// variables related to the size of the word size used e.g. 16, 32 or 64 bit
int number_of_bits_in_a_word = 32;		// number of bits in a word (NOTE: Although this is defined here, there are other places that a 32 bit word is assummed)
int number_of_bytes_in_a_word = 0;		// number of bits in a word, depends on computer

// variables related to the size of the input vector
int packetSize;							// declares the number of bytes in incoming bit vector, I, can be input (or calculated if it's a CCSDS space packet)
int Num_of_words; 						// declares number of words in buffer needed to process the incoming bit vector, I
int End;								// declares the bit position of the end of the vectors I, B, M etc

// variables related to tracking the input and output bit position during reading and writing 
int old_bit_position;		// the present bit position of the last processing step in the input bit vector
int o_bit = 0;				// the present write position in the present word of the output string
int o_word = 0;				// the present word being written to in the output string, set initially with MSB bit set

// variables related to storing and tracking the history of the masks 
int no_mask_change[16] = {0};       // array to store the history last 16 iterations if they had a mask change or not, pt_history_index used as a tracker
int pt_history[16] = {0};           // array to store the history last 16 iterations if the pt flag was set to one or not, pt_history_index used as a tracker
int Dt_history_index = 7;			// tracks where we are in the 7 last Dt stored arrays
int pt_history_index = 7;			// tracks where we are in the no_mask_change and pt_history arrays
int pt_history_index_switch = 8;	// needed to switch pt_history_index between Dt_history_index which counts to 7 (associated wuoth Rt, and Vt which counts to 15)

// DEFINE USER INPUT PARAMETERS, WILLL BE SET VIA INPUT COMMAND LINE
int Rt	= 0;				// robust level (0-7)
int pt 	= 0;				// new mask required (boolean)
int ft	= 1;				// mask needs to be sent required (boolean)
int rt	= 1;				// uncompressed data should be sent (boolean)

// DEFINE FUNCTIONS
// Input and Output only
int readBinaryFile(const char * path, uint8_t ** sourcePackets);
void getNextPacket(uint8_t * sourcePackets, uint8_t ** Packet, int packetNumber, int packetSize);
void write_to_file(unsigned int o[], FILE * outputFile);

// Actual compression algorithm functions
void compress(unsigned int I[], unsigned int I_old[], unsigned int M[], unsigned int B[], unsigned int Y[Rt][Num_of_words], unsigned int o[], unsigned int end_o, unsigned int initial);
void encode_count(unsigned int x, int word, unsigned int o[]);
void concat(uint32_t value, int length, unsigned int o[]);
void write_count(unsigned int delta, unsigned int o[]);

/////////////////////
// MAIN
////////////////////
int main(int argc, char** argv)  {

	///////////////////////////
	/// collect input arguments
	////////////////////////////
	char inputFileName0[64] = {0};				// defines the input file name
	strcpy(inputFileName0, argv[1]);		// copies the first argument as the input file name
	packetSize = atoi(argv[2]);				// copies the second argument as the length of the packets in the input file in bytes (if it is set to 1 then it assumes a CCSDS Space Packet format and will calculate the length using that field)
	int pt_limit = atoi(argv[3]);			// copies the third argument as the period in which subsequent new mask flag (pt) is set (if zero, the flag is never set)
	int ft_limit = atoi(argv[4]);			// copies the fourth argument as the period in which subsequent send mask flag (ft) is set (if zero, the flag is never set)
	int rt_limit = atoi(argv[5]);			// copies the fifth argument as the period in which subsequent uncompressed packet flag (rt) is set (if zero, the flag is never set)
	Rt = atoi(argv[6]);						// copies the sixth argument as the minimum robustness level (0 means a single packet loss will stop the decompression, 1 means two sequential packet losses will stop the decompression, 2 means three sequential packet losses will stop the decompression etc


	////////////////////////////////////////////////////////
	// INPUT THE FILE CONTAINING THE BIT VECTORS TO COMPRESS INTO binary array called BinaryFile0
	/////////////////////////////////////////////////////////
	uint8_t *binaryFile0 = NULL;
	int fileSize0 = readBinaryFile(inputFileName0, &binaryFile0);			// read in the file as a single byte array


	//////////////////////
	// IF THE USER SET THE PACKET SIZE TO ONE THE FILE CONTAINS CCSDS PACKETS, SO WE CAN GET PACKETLENGTH FROM LENGTH FIELD OF FIRST PACKET
	//////////////////////
	if (packetSize == 1) {
		packetSize = ((binaryFile0[4] << 8) + binaryFile0[5] + 7);		// reads the length field of the CCSDS packet and sets Packetsize to equal this plus one plus the header
	}

	// DEFINE PACKET BUFFER
	uint8_t *Packet0 = NULL;

	///////////////////
	// DECLARATIONS	///
	///////////////////
	number_of_bytes_in_a_word = number_of_bits_in_a_word/8;		// number of bits in a word, depends on computer
	Num_of_words = (packetSize + 3 ) / 4;		// assumes 32 bit words
	End = packetSize*8;							// defines the bit position of the end of the bit vectors
	int pt_counter = pt_limit;				// initialises the counter tracking the frequency of the new mask flag setting
	int ft_counter = ft_limit;				// initialises the counter tracking the frequency of the send mask flag setting
	int rt_counter = rt_limit;				// initialises the counter tracking the frequency of the uncompressed packet flag setting

	/// CREATE BIT VECTORS
	unsigned int I[Num_of_words];			// Buffer for the input packet to be compressed
	unsigned int I_old[Num_of_words];		// Buffer for the last input packet
	unsigned int M[Num_of_words];			// Buffer for the mask
	unsigned int B[Num_of_words];			// Buffer for the mask under construction
	unsigned int Dt_history[(7+1)][Num_of_words];	// 2D Buffer for the historical mask updates

	// SET ALL BIT VECTORS TO ZERO
	memset(I, 0, Num_of_words*number_of_bytes_in_a_word);
	memset(I_old, 0, Num_of_words*number_of_bytes_in_a_word);
	memset(M, 0, Num_of_words*number_of_bytes_in_a_word);
	memset(B, 0, Num_of_words*number_of_bytes_in_a_word);
	memset(Dt_history, 0, Num_of_words*number_of_bytes_in_a_word*(8));


	// CREATE AND INITIALISE OUTPUT BIT STRING
	int numberOfPackets;
	unsigned int end_o = 6*Num_of_words;	// Set output buffer to 6 times the size of the uncompressed packet. Checked in concat subroutine so if change here, change there.
	unsigned int o[end_o];			// Buffer for the output bit string
	o[0] = 0;						// Set the first word of the output bit string to zero


	/////////////////////
	// CALCULATE THE NUMBER OF PACKETS IN THE FILE
	////////////////////
	numberOfPackets = fileSize0/(packetSize);

	//////////////////////
	// CREATE THE OUTPUT FILE TO WHICH THE OUTPUT STRING WILL BE WRITTEN, SAME AS INPUT FILENAME WITH A PKT EXTENSION
	//////////////////////
	char outfile[50];
	strcpy (outfile, inputFileName0);
	strcat(outfile, ".pkt");
	FILE * outputFile = fopen(outfile, "wb");


	////////////////////////
	// READS THE FIRST PACKET AS A BYTE ARRAY CALLED Packet0
	////////////////////////

	getNextPacket(binaryFile0, &Packet0, 1, packetSize);	// calls subroutine to return the first packet byte array from the file byte array

	////////////////////////
	// CREATE I_old from the first packet byte array
	///////////////////////
	int j = 4;												// set the counter for which byte it is to 4
	uint32_t bytesToInt = 0;								// create a variable to store the word value created from the 4 bytes
	int currentWord = 0;									// set the current word index of I_old to zero
	for(int currentByte = 0; currentByte < packetSize; currentByte++ ){		// increment through the bytes in the  byte array
		j -= 1;													// decrement the counter for which byte it is
		bytesToInt |= Packet0[currentByte] << (j * 8);			// Update the 32bit variable which stores the 4 byte set with the next byte value
		if (j == 0) {											// when four bytes have been processed
			I_old[currentWord] = bytesToInt;					// set the I_old word to be equal to this
			currentWord +=1;									// increment the word for I_old
			bytesToInt = 0;										// set the 32 bit variable to create the word to zero
			j = 4;												// set the counter for which byte it is to 4
		}
	}
	if( (j < 4) ){												// If the word is incomplete
		I_old[currentWord] = bytesToInt;						// then set the last word anyway i.e. it will have trailing zeros.
	}


	//////////////////////////////////////////////////////////////
	/// compress the first packet with initialisation set to one
	///////////////////////////////////////////////////////////////
	compress(I, I_old, M, B, Dt_history, o, end_o, 1);
	write_to_file(o, outputFile);					// writes output string to file


	////////////////////
	//  MAIN LOOP	////
	////////////////////
	for(int i = 2; i <= numberOfPackets; i++){		// Loops through the rest of the packets in the file

		getNextPacket(binaryFile0, &Packet0, i, packetSize); // get next packet byte array from the file byte array

		////////////////////////
		// Create the input word array, I, from the packet byte array
		///////////////////////
		int j = 4;
		bytesToInt = 0; 				// create a 32bit unsigned integer variable to store the word value created from the 4 bytes
		int currentWord = 0;			// set the current word index of I to zero
		for(int currentByte = 0; currentByte < packetSize; currentByte++ ){ 	// increment through the bytes in the byte array
			j -= 1;						// decrement the counter for which byte it is
			bytesToInt |= Packet0[currentByte] << (j * 8);  // Update the 32bit variable which stores the 4 byte set with the next byte value
			if (j == 0) {									// when four bytes have been processed
				I[currentWord] = bytesToInt;				// set the I word to be equal to this
				currentWord +=1;							// increment the word for I_old
				bytesToInt = 0;								// set the 32 bit variable to create the word to zero
				j = 4;										// set the counter for which byte it is to 4
			}
		}

		if( (j < 4) ){						// deal with incomplete word
			I[currentWord] = bytesToInt;	// then set the last word anyway i.e. it will have trailing zeros.
		}

		////////////////////////////////////
		//	Increments the send full mask counter which sets the send mask flag, ft, periodically
		//////////////////////////////////////
		if (ft_counter==1) {
			ft=1;						// set the send mask flag to one
			ft_counter=ft_limit;		// reset the counter to maximum again
		} else {
			ft_counter--;				// decrement the counter
			ft=0;						// set the send mask flag counter to zero
		}

		////////////////////////////////////
		//	Increments the positive update counter which sets the positive update fiag, pt, periodically
		//////////////////////////////////////
		if (pt_counter==1) {
			pt=1;						// set the update mask flag to zero
			pt_counter=pt_limit;		// reset the counter to maximum again
		} else {
			pt_counter--;				// decrement the counter
			pt=0;						// set the update mask flag counter to one
		}


		////////////////////////////////////
		//	Increments the send entire packet in the clear flag, rt, periodically
		//////////////////////////////////////
		if (rt_counter==1) {
			rt=1;						// set the uncompressed flag to one
			rt_counter=rt_limit;		// reset the counter to maximum again
		} else {
			rt_counter--;				// decrement the counter
			rt=0;						// set the uncompressed flag counter to zero
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// keep rt and ft flags fixed to one and pt to zero for the first Rt+1 packets, see PARAMETER section of standard //
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (i<Rt+2) {
			ft=1;
			rt=1;
			pt=0;
		}

		////////////////////////
		// Calls the compression algorithm with all the inputs
		// I = Present packet to compress
		// I_old = Last packet that was compressed
		// M = Present Mask
		// B = Mask under construction
		// Dt_history = Dt-Rt....Dt i.e history of mask changes
		// o = output bit vector
		// end_o = end position of the output buffer in bits
		// initialisation flag, 1=first packet, 0=next packet
		///////////////////////
		compress(I, I_old, M, B, Dt_history, o, end_o, 0);

		////////////////////////////////
		// write output vector for this iteration to the output file
		////////////////////////////////
		write_to_file(o, outputFile);				


	}
	///////////////////////
	// END OF PACKET LOOP//
	///////////////////////

	printf("%s \n", "SUCCESS! " );
	return EXIT_SUCCESS;
}

/////////////////
// END OF MAIN //
/////////////////


///////////////////////////////////////////////////////////
// Function COMPRESS: 
// This is the function that will compress the input vector I. 
// The first loop cycles through the words in the buffers (M, I, Iold, B) to
// 1) update the buffers and store for the next iterationan
// 2) Create Dt, update Dt_history, create Xt and RLE Xt to write counters to the output bit vector. It also creates yt and writes it at the end of the output buffer for potential later use 
// 3) terminates the counters with "10" 
// The part between the first and second loops:
// 1) Updates the two arrays detailing if pt was set to one, or no mask changes occured for this iteration
// 2) Calculates and then appends the real robustness level, Vt, to the output bit vector
// 3) Calculates and appends, et, kt, ct, dt to the output vector  
// 4) Increments to history array pointers
// The second loop cycles through the words in the buffer (M) to sent a compressed version of the entire mask, qt, if ft is set to one
// The third loop cycles through the words in the buffers to:
// 1) Send an entire copy of I, if rt = 1
// 2) Send all the values of the non predicatable bits, if rt = 0
// 3) Send all the values of the bits that have been changed from non predictable to predictable in Xt, if ct = 1 i.e. two positive mask changes duiring Vt period
////////////////////////////////////////////////////////////////


void compress(unsigned int I[], unsigned int I_old[], unsigned int M[], unsigned int B[], unsigned int Dt_history[Rt][Num_of_words], unsigned int o[], unsigned int end_o, unsigned int initial) {

	//////////////////////
	// DEFINITIONS     ///
	//////////////////////
	int	flag_no_pos_updates_in_Xt = 1;				// initialises flag to say no positive mask updates in Xt to be true (1)
	int	flag_no_mask_updates_in_Xt = 1;				// initialises flag to say no mask updates at all in Xt to be true (1)  
	int	NP_bit_value_write_position;									// used to write the NP bits
	unsigned int x;									// common loop variable
	unsigned int change = 0;						// common loop variable
	unsigned y_bit = 0;								// y_word is the start position to write yt from LSBit to MSBit, starts at end of output vector as a buffer area
	unsigned y_word = end_o-1;						// y_word is the start position to write yt from LSByte to MSByte, starts at end of output vector as a buffer place
	int start;										// general variable to deal with loop start point
	int Vt;											// variable for the actual robustness level, Vt in the standard

	unsigned int X[Num_of_words];					// Buffer for the changes in the mask, used for Dt and then later Xt in standard
	memset(X, 0, Num_of_words*number_of_bytes_in_a_word); // clear the X buffer

	//////////////////////
	// RESET VARIABLES ///
	//////////////////////
	o[y_word]=0;									// clears last output word
	o[0] = 0;										// clears first output word
	o_bit = 0;										// sets the bit write counter for the output to zero
	o_word = 0;										// sets the output word counter to zero
	int dt=0;										// variable to show dt in standard
	int x_end;										// general variable for loops
	int Rt_mod = 0;									// variable to say that Rt=0 but Vt > 0, set to FALSE as default
	int flag_no_mask_updates_in_Dt = 1;				// flag to record if no mask changes in Dt were found, set to true (1) as default
	int ct = 0;										// variable ct in the standard, set to zero as default

	// calculates the array pointer uesed in the no mask change and pt history array as Dt_history_index counts from 7 to zero but pt_history_index counts from 15 to zero, switch determines which case it is 
	pt_history_index=Dt_history_index+pt_history_index_switch;


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Looks for the special case where Rt=0 but Vt > zero. This needs to be flagged before Vt is calculared			   					      		//	
	// If Rt is zero AND there were no mask changes in the last iteration (recorded in no_mask_change array) then Vt>0, recorded in the Rt_mod variable //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (Rt==0) {
		if (pt_history_index==15) {       // deals with end of array case
			start = 0;
		} else {
			start = pt_history_index+1;   // looks at the entry before the present pointer 
		}
		if (no_mask_change[start] == 1) {Rt_mod=1;}    // if the entry in the no_mask_change array was 1 i.e. there were no changes, then Vt has to be calculated and a flag is set to ensure that later
	}


	////////////////////////////////////////////////////////////////
	// START OF FIRST WORD LOOP                              	///
	////////////////////////////////////////////////////////////////

	old_bit_position = End;							                // set bit position to the end of the vector before encoding the counters in the following loop
	for(int word = Num_of_words-1; word >= 0; word--){				// sets up word loop

		if (initial==0) {

			/////////////////////////////////////////////////////////////////////////////////////////////////////////
			// UPDATE MASK, BUILD, CALCULATE NEGATIVE MASK UPDATES, UPDATE OLD INPUT WITH NEW INPUT  (EQ 6 and 7) ///
			/////////////////////////////////////////////////////////////////////////////////////////////////////////
			I[word] ^= I_old[word];						// calculate XOR of the old and new input word
			I_old[word]  ^= I[word];					// the old input is updated with the new input word
			B[word] |= I[word];  						// updates new mask being built with new bits that have changed 

			if (pt == 0) {								// IF NO MASK UPDATE WAS REQUESTED
				I[word] |= M[word];						// calculates an updated mask with new bits that have changed and stores it in I[word]
				X[word] = I[word] ^ M[word]; 			// calculates the differences between old mask and updated mask i.e. Dt in EQ 8, here X = Dt, same variable is used later when X = Xt
				M[word] = I[word];						// updates the mask with this new changes
			} else {									// IF A MASK UPDATE WAS REQUESTED
				X[word] = M[word] ^ B[word]; 			// calculates the differences between old mask and updated mask i.e. Dt in EQ 8, here X = Dt, same variable is used later when X = Xt 
				M[word] = B[word]; 						// sets the mask to be the same as build
				B[word] = 0; 	            			// resets the new mask to zero
			}


			////////////////////////////////////////////////////////////////////////////////////
			// Sets flag_no_mask_updates_in_Dt to zero to record that at least one mask change has occured in Dt //                                                                                                ///
			////////////////////////////////////////////////////////////////////////////////////
			if (X[word] != 0) {flag_no_mask_updates_in_Dt = 0;}			// A mask change has been found, so set flag_no_mask_updates_in_Dt to zero to record it for later
			
			
			///////////////////////////////////////////////////////////////////////////////////////////
			// Update the next position in the Dt history array, needed in EQ 16 to calculate Xt ///
			///////////////////////////////////////////////////////////////////////////////////////////
			Dt_history[Dt_history_index][word] = X[word];					// save the mask change result into the current historical mask change buffer i.e D in the standard 


			// Calculates start which defines how many of the historical Dt values in the array (Dt_history) will be ORed together to form Xt (EQ 16) 
			start = Dt_history_index+Rt;

			/////////////////////////////////////////////////////////////////////////////////
			// ORs through Dt historical array (Dt_history), to produce <X> in the standard (EQ 16) ///
			/////////////////////////////////////////////////////////////////////////////////
			if (Rt>0) {	  								// if Rt will be greater then zero
				if (start>7) {start = start-8;} 		// deal with buffer wrap around
				for(int i = Dt_history_index; i <= 7; i++) {

					X[word] = X[word] | Dt_history[i][word];   	// ORs all the entries in the historical mask update buffers covering Rt to zero iterations to produce <X> in the standard (EQ 16)
					if (i==start) {break;}				// exit if we have reached the start
					if (i==7) {i=-1;}					// deal with buffer wrap around
				}
			}
		}




		////////////////////////
		// Create ht (EQ 15) ///
		////////////////////////

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// READS <X> backwards to produce X and WRITES MASK CHANGE COUNTERS TO OUTPUT BIT STRING TO OUTPUT BUFFER i.e. RLE(X) in the standard (EQ 15)   ///
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		change = X[word]; 	        		 // copies X into a temp variable called change

		while (change) {  					// iterate until no more set bits in change exist
			flag_no_mask_updates_in_Xt = 0;						// changes have been found, so set no_mask_updates flag is now false i.e zero
			x= change & -change; 			// mechanism to find the LSB set in the change variable
			encode_count(x, word, o);		// calls a subroutine to determine and encode the delta counter and concantenate it with the output bit string
			change -= x; 					// mechanism to remove the LSB in change and so iterate through the word




			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Calculate a bit vector with the actual values of all the changed bits in Xt i.e yt in the standard (EQ 17), which might become kt (EQ 19)
			// Also store in flag_no_pos_updates_in_Xt if there are any positive mask updates i.e  yt is only zeros
			// yt is stored where at the end of the output buffer and written backwards, hence yt is actually written from the MSB to LSB as normal
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if ((Rt>0) || (Rt_mod==1)) {	  	// if Vt will be greater then zero
				if ((x & M[word])==0) {			// if the mask bit value at that position is 0 i.e. positive update
					flag_no_pos_updates_in_Xt = 0;	// set flag to say there is at least one positive mask update in Xt found
					o[y_word] |= 1<<y_bit;		// left bit shift the one to the write position OR it with o to insert a one for a positive update
				}
				y_bit++;							// increment the e write bit position in the word

				if (y_bit == number_of_bits_in_a_word) {	// deal with a word boundary in the output vector
					y_word--;								// decrement the word
					o[y_word]=0;							// set the word to zero
					y_bit = 0;								// reset the counter
				}
			}
		}
	}
	
	/////////////////////////
	// END OF FIRST WORD LOOP
	//////////////////////////



	////////////////////////////////////////////////////////////////
	// BETWEEN FIRST AND SECOND WORD LOOP                 
    // 1) Updates the two arrays detailing if pt was set to one, or no mask changes occured for this iteration
    // 2) Calculates and then appends the real robustness level, Vt, to the output bit vector
    // 3) Calculates and appends, et, kt, ct, dt to the output vector  
    // 4) Increments to history array pointers       
	////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////
	// WRITES '10' to terminate RLE(Xt) as described in the RLE section of the standard //
	//////////////////////////////////////////////////////////////////////////////////////
	concat(2, 2, o); // Concatenate "10" terminator in standard

	// Update the no_mask_change historical array with the final result on if any mask changes were found or not
	no_mask_change[pt_history_index] = flag_no_mask_updates_in_Dt;
	if (initial==1) {no_mask_change[pt_history_index] = 1;} // covers initialisation case

	// Update the positive mask change historical array with the value of the positive change flag for this iteration
	pt_history[pt_history_index] = pt;

	/////////////////////////////////////////
	// Calculate  Vt in standard
	/////////////////////////////////////////
	Vt=Rt; // initialise to Rt

	start = pt_history_index+Rt+1;   // calculate the first buffer position before present position - Rt
	if (start>15) {start = start - 16;} // deal with wrap around at 15

	// Increment through the buffers starting at first buffer position before present position and incrementing for a maximum of 15
	for(int i = 0; i < (15-Rt); i++){
		if (start>15) {start=0;}  // // deal with wrap around at 15
		if (no_mask_change[start] == 1) {
				Vt++;  // no mask change so increment V
		} else {
				break;  // found a mask change exit
		}
		start++;  // move to next buffer
	}


	/////////////////////////////////////////////////
	// Concatenate "BIT_4(Vt)" in standard (EQ 15) //
	/////////////////////////////////////////////////
	concat(Vt, 4, o);

	///////////////////////////////////////
	// Calculates et, Kt and ct in standard
	///////////////////////////////////////
	if ((Vt > 0) && (flag_no_mask_updates_in_Xt==0)) {  // if Vt is zero, or there were no mask changes then et, kt and ct are all null vectors

		if (flag_no_pos_updates_in_Xt==1) { 			// if there are no positive changes in Xt then et will equal 0 and kt and ct are null vectors
			concat(0, 1, o);			// Set et to zero and we concatenateit to the output, (EQ 18), kt and ct are null
			ct=0;						// set ct to zero, to flag this case later
		} else {
			concat(1, 1, o);			// Set et to one and we concatenate it to the output, (EQ 18), we continue with kt creation


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// We stored yt earlier at the end of the output vector															 //		
			// Now we concatenate it at the present position of the output vector i.e. kt (EQ 19)   						 //
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			if (y_bit >0) { concat(o[y_word], y_bit,o);}  // writes incomplete word
			y_word++;
			while (y_word < end_o) {
			concat(o[y_word], number_of_bits_in_a_word,o); // writes any full words
				y_word++;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Now we run thround the historical pt value_history array looking to find if it was set twice in the period covering Vt (EQ 20) 
			// If it has then we set ct to one otherwise it is 0 and we append it to the output vector
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			start = pt_history_index+Vt; // calculates present position plus Vt
			x_end=0;					// variable to count the number of times pt is set
			if (start>15) {start = start-16;} // deal with buffer wrap around

			for(int i = pt_history_index; i <= 15; i++) {
				if (pt_history[i] == 1) {x_end++;}
				if (i==start) {break;}
				if (i==15) {i=-1;}
			}

			if (x_end>1) { 				/// if pt was set more then once the ct=1
					ct=1;
					concat(1, 1, o); 	// Concatenate ct=1 to output vector
			} else {
				    concat(0, 1, o); 		// Concatenate ct-0 to output vector
					ct=0;
			}
		}
	}


	////////////////////////////////////////////////
	// Calculate and write dt in standard (EQ 13) //
	////////////////////////////////////////////////
	if ((ft==0) && (rt==0)) {
		dt=1;
		concat(1, 1, o);     // (EQ 13, 1st case) neither a full mask or a vector in the clear will be sent
	} else {
		concat(0, 1, o);	// (EQ 13, 2nd case) either a full mask or a vector in the clear or both will be sent
	}


	/////////////////////////////////////////////////
	// Increments the index for Dt history buffer. Also changes pt_history_index_switch if Dt_history_index is 7 as pt_history is 15 not 7///
	/////////////////////////////////////////////////
	if (Dt_history_index==0) {		// if the last mask change history buffer has just been filled restart from the top one
		Dt_history_index = 7;		// set the next buffer point for the negative mask updates to equal the robustness level
		if (pt_history_index_switch == 8) {pt_history_index_switch = 0; } else {pt_history_index_switch = 8;}	// this is the switch to account for the Vt buffer being 8 more than the Rt buffer
	} else {				// otherwise move to the next one down
		Dt_history_index--;			// Decrements the next buffer point for the mask changes
	}



	////////////////////////////////////////////////////
	// START OF SECOND WORD LOOP : Create qt (EQ 21) ///
    // Cycles through the words in the buffer (M) to sent a compressed version of the entire mask, qt, if ft is set to one
	////////////////////////////////////////////////////

	if (dt==0) {													// This is the case that dt was zero meaning either a full mask or vector in clear or both will be sent

		if (ft == 0) {
			concat(0, 1, o);										// The full mask is not set so qt is simply zero and appended to the output vector (EQ 21, 3rd case)  
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// The full mask is preprocessed by bit shifting the entire mask to the left by one bit, then XORing the result with itself, then RLE encoding the result  //
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (ft == 1) {												// The full mask flag is set 
			concat(1, 1, o);										// Append a one to output vector, EQ 21, 2nd case) 

			old_bit_position = End;	// set bit position to the end of the vector before encoding the counters in the next loop

			/////////////////////////////////////////////////////////////////////////////////////////
			// bit shifting the entire mask to the left by one bit, then XORing the result with itself
			//////////////////////////////////////////////////////////////////////////////////////////
			for(int word = Num_of_words-1; word >= 0; word--){		// sets up word loop
				change = M[word] << 1;								// shifts the mask to the left
				if (word != Num_of_words-1) {
					change = change | (M[word+1]>>31);					// OR it together the left shifted mask
				}
				change = change ^ M[word];								// perform an XOR of the left shifted mask and the original mask


				//////////////////////////////////////
				// RLE of the result and append to output vector
				//////////////////////////////////////
				while (change) {							// iterate until no more set bits in change exist
					x = change & -change; 					// mechanism to find the LSB set in the change variable
					encode_count(x, word, o);				// calls a subroutine to determine and encode the delta counter and concantenate it with the output bit string delta counter and write it to the output bit string
					change -= x;							// mechanism to remove the LSB in change and so iterate through the word
				}
			}

				concat(2, 2, o);				// "10" i.e. zero count, signals the end of the change mask update counters

		}

	}
	////////////////////////
	// END OF second LOOP
	////////////////////////



	///////////////////////////////////
	// Start of 3rd lOOP : Create ut (EQ 22) 
    // The third loop cycles through the words in the buffers to:
    // 1) Send an entire copy of I, if rt = 1, else
    // 2) Send all the values of the non predicatable bits, and
    // 3) Send all the values of the bits that have been changed from non predictable to predictable in Xt, if ct = 1 i.e. two positive mask changes duiring Vt period
	///////////////////////////////////

	if (rt) {		// if uncompressed flag is set to one, then all bits are written to the output bit string

		concat(1, 1, o);					// Signals the entire input vector will be sent, (EQ 22, case 3)
		write_count((End), o);		       // Encodes the bit length of the input vector (COUNT (F), and concatenates it with the output bit string (EQ 22, case 3)

		////////////////////////////////
		// Writes the entire input vector to the output bit vector (EQ 22, case 3)
		////////////////////////////////
		for(int word = 0; word<Num_of_words; word++) {		// write all the words of the incoming bit string to the output bit string
			concat(I_old[word], number_of_bits_in_a_word, o);	// write whole words to output bit string
		}

			///////////////////////////////////
			// clean up as we might have a mismatch between the word boundary and the byte boundary
			// move o_bit and o_word backwards to compensate for word/byte mismatch (as the routine above writes words not bytes) i.e. zeros were added in the byte to word conversion
			//////////////////////////////////
			int back = Num_of_words*32-End;			// calculate how many zeros were added
				if (o_bit>=(back+1)) {				// if the present bit position in o is larger then the number of zeros added simply decrement it
					o_bit=o_bit-back;				// decrement the o bit position
				} else {							// we need to go back one word
					o_word--;						// decrement the o word position
					o_bit = 32+o_bit-back;			// calculate the new o bit position
				}


	} else {

		///////////////////////////////////////////////////////////////////////////////
		// Covers the first bit of (EQ 22, case 4 and 5), i.e. dt was zero, but a vector in the clear will not be sent as rt (full mask flag) was not set
		// Note cases 1 and 2 do not require this bit and so are covered 
		///////////////////////////////////////////////////////////////////////////////
		if (dt==0) {								// if not the case of special
			concat(0, 1, o);						// signal only NP bit values will be sent
		}


		////////////////////////////////////////////////////////////////////////
		// Covers (EQ 22, case 1,2,4 and 5) wrt to BE functions. They are indentical except for the impact of flag ct being set to one
		// if ct is set to one then the actual values of the bits set as positive in Xt are captured as well as the unpredictable bits (done by ORING the inverse of Xt with Mt)
		////////////////////////////////////////////////////////////////////////

		NP_bit_value_write_position = 32-o_bit;						// initilise the Non Predicatble Bit Value write bit position in the word
		for(int word = Num_of_words-1; word >= 0; word--){				// sets up word loop 

			if (ct==0) {
				change = M[word]; 											// create a copy of the mask word to avoid modifying it (EQ 22, case 2 and 5)
			} else {
				change = M[word] | X[word]; 								// ORs Mt with the change word Xt, so including the positive changes as well (EQ 22, case 1 and 4)
			}


			//////////////////////////////////////////////////////////
			// BE funtion described in the standard, outputting the actual values of I when change shows a one in that bit position (EQ 11)
            // For efficiency we do not use the write_count function to write 1s and 0s here because we know they are all single bits
            // NP_bit_value_write_position, is used to increment the write position through the output bit vector from MSbit to LSbit 
            // writing 1s when the corresponding bit in I is a one and just moving the write position otherwise 
			//////////////////////////////////////////////////////////
			while (change) {											// iterate until no more set bits in copy exist
				x= change & -change;  									// mechanism to find the LSB set in the copy variable, derived from Mt
				NP_bit_value_write_position--;							// decrement the NP write bit position in the word, which as it is left shifted in the next step moves it forward
				if (x & I_old[word]) {									// if the bit value of the input bit vector at that position is 1, in the event of a zero no action is required
					o[o_word] |= (1<<NP_bit_value_write_position);		// left bit shift the one to the write position OR it with the output word to insert a one
				}


				if (NP_bit_value_write_position == 0) {			            // deal with a word boundary in output bit vector
					o_word++;								                    // increment the output word
					o[o_word]=0;							                    // sets the new word to zero
					o_bit=0;								                    // sets the new bit position to zero
					NP_bit_value_write_position = number_of_bits_in_a_word;    // reset the write position to maximum, so MSBit
				}
				change -= x; 									                // mechanism to remove LSB in copy and so iterate through the word
			}
		}
		o_bit=32-NP_bit_value_write_position;								    // set output buffer write position back to the correct position

	}

	////////////////////////
	// END OF 3rd LOOP   //
	///////////////////////

}





/////////////////////////////////////////////////////////////////////////////////////////////
// Function calculates the absolute bit position of the set bit in the bit vector          //
// Compares this to the old bit position to determine a delta 			                    //
// Calls write_count function to append the coded value of this delta to the output vector  //
//////////////////////////////////////////////////////////////////////////////////////////////
void encode_count(unsigned int x, int word, unsigned int o[]) {
	uint32_t MultiplyDeBruijnBitPosition[32] = {1, 2, 29, 3, 30, 15, 25, 4, 31, 23, 21, 16, 26, 18, 5, 9, 32, 28, 14, 24, 22, 20, 17, 8, 27, 13, 19, 7, 12, 6, 11, 10
};
	int bit_position_in_word = MultiplyDeBruijnBitPosition[((uint32_t)(x * 0x077CB531U)) >> 27];  	// note sometimes native low level processor instructions can perform this step
	bit_position_in_word=32-bit_position_in_word; 													// count from the other side

	int new_bit_position = word*number_of_bits_in_a_word + bit_position_in_word;					// calculate the bit position of set bit in vector

	int delta = old_bit_position - new_bit_position;												// calculate the delta between the last position and this one
	old_bit_position = new_bit_position;															// update old bit position

	write_count(delta, o);																			// write it to the output packet

}


/////////////////////////////////////////////////////////////////////////////////////////////
// Function to encode a value, delta, according to the counter encoding function of the standard    //
// Then writes this encoded value to the output vector  //
//////////////////////////////////////////////////////////////////////////////////////////////
void write_count(unsigned int delta, unsigned int o[]) {

	if (delta == 1) {								// if delta = 1, counter function says write a zero
		if (o_bit<(number_of_bits_in_a_word-1)) {	// if no word boundary is more than one bit away then
			o_bit++;								// just increment write pointer to write a zero
		} else {   									// else write a zero by jumping to next word
			o_word++;				// If a word boundary in one bit then move to next word to create the zero
			o[o_word]=0;			// clears next word
			o_bit=0; 				// sets write pointer to zero
		}
		return;
	}

	// Since zero has a special code for end of counters, and one is coded as a single zero, we can remove two from the delta value before encoding
	int delta2 = delta-2;

	///////////////////////////////////////////////////
	// Implements Table 5-1 and EQ 9
	// ORs a particular number with delta2 (delta-2)
	// Note the particular number is chosen to have the corresponding bit length and to start with a "110" if delta2 < 32, or "111" if above
	// Once the OR is done it calls the Concat function which will write this to the output vector using the requested number of bits 
	///////////////////////////////////////////////////
	if (delta2 < 32) {concat( 192 | delta2, 8, o); return;}						// "11000000" ORed with delta2 (8 bits total)
	if (delta2 < 64) {concat(448 | delta2, 9, o);  return;}						// "11100000" ORed with delta2 (9 bits total)
	if (delta2 < 128) {concat(1792 | delta2, 11, o); return;}					// "1110000000" ORed with delta2 (11 bits total)
	if (delta2 < 256) {concat(7168 | delta2, 13, o); return;}					// "111000000000" ORed with delta2 (13 bits total)
	if (delta2 < 512) {concat(28672 | delta2, 15, o); return;}					// etc
	if (delta2 < 1024) {concat(114688  | delta2, 17, o); return;}				
	if (delta2 < 2048) {concat(458752  | delta2, 19, o); return;}				
	if (delta2 < 4112) {concat(1835008  | delta2, 21, o); return;}				
	if (delta2 < 8192) {concat(7340032 | delta2, 23, o); return;}				
	if (delta2 < 16384) {concat(29360128 | delta2, 25, o); return;}				
	if (delta2 < 32768) {concat(117440512  | delta2, 27, o); return;}			
	if (delta2 < 65536) {concat(469762048  | delta2, 29, o); return;}			
	if (delta2 < 131072) {concat(1879048192  | delta2, 31, o); return;}			
	if (delta2 > 131071) printf("%s", "oops, trying to wrtie a counter that is too long"); return; 											// ERROR bit string too long
}


///////////////////////////////////////////////////////////////////////////////////
// Function to write a input number with a requested number of bits to the output bit vector
///////////////////////////////////////////////////////////////////////////////////
void concat(uint32_t value, int length, unsigned int o[]) {
  if (o_word<4*Num_of_words) {
	int carry = length + o_bit - number_of_bits_in_a_word;		// calculate the carry over, if the bit string will not all fit in the present word in the output string
	if (carry > 0) {  						// indicates that we need to carry over to the next word
		uint32_t mask = value >> carry;					// right shift value the carry number of bits to create a mask
		o[o_word] = o[o_word] | mask;				// OR the mask with the present word of the output string
		value = value << (number_of_bits_in_a_word-carry);	// left shift value to bring the rest of the bit string to start at the MSB position
		o_word++;						// move to the next word in the output string
		o[o_word] = value;					// set the word to equal value
		o_bit = carry;						// set the next write bit position of the output string to equal the carry position
	} else {							// the whole bit string can fit in the present word
 		value = value << -1*carry;				// left shift value so its MSB is aligned with the next write bit position in the output string
		o[o_word] = o[o_word] | value;				// OR value with the present word of the output string
		if (carry) { 						// if the bit string does not finish the present word
			o_bit = o_bit+length;				// update the write position
		} else {						// the bit string exactly finishes the present word
			o_word++;					// move to the next word in the output string
			o[o_word]=0;					// clears next word
			o_bit = 0;					// set the write position to the zero position
		}
	}

  } else {

	  printf("%s", "oops, trying to write a packet greater than 4 x the size of the input packet");
	  exit(0);

  }

return;
}

/////////////////////////////////////////////////////////////////////////////////////
// Function to read the whole input file into a byte array called source packets ////
/////////////////////////////////////////////////////////////////////////////////////
int readBinaryFile(const char * path, uint8_t ** sourcePackets){

	*sourcePackets = NULL; //point to the NULLSPACE

	FILE * pFile; //filedescriptor for opening the file

	pFile = fopen ( path , "rb" ); //open the file in read and binary mode
	if (pFile==NULL) {fputs ("File error\n",stderr); return 0;}

	/* obtain file size */
	int lSize; //size of the opened file
	fseek (pFile , 0 , SEEK_END); // put the pointer to the end of the file descriptor
	lSize = ftell (pFile); //get the size of the opened file
	rewind (pFile); //pointer goes back to the beginning

	/* allocate memory to contain the whole file: */
	*sourcePackets = malloc(lSize);
	if (*sourcePackets == NULL) {fputs ("Memory error",stderr); return 0;}

	/* copy the file into the sourcePackets */
	int result;
	result = fread (*sourcePackets, 1, lSize, pFile);
	if (result != lSize) {fputs ("Reading error\n",stderr); return 0;}
	fclose (pFile); //close the filedescriptor

	/* the whole file is now loaded in the memory. */

	return lSize; //return the allocated memory
}

////////////////////////////////////////////////////////////////////////////////////////////
// Function to copy data from the source packets byte array to a new packet byte array  ////
////////////////////////////////////////////////////////////////////////////////////////////
void getNextPacket(uint8_t * sourcePackets, uint8_t ** Packet, int packetNumber, int packetSize){
	if (*Packet == NULL){
		*Packet = malloc(packetSize);
		if (*Packet == NULL) {fputs ("Memory error",stderr); return;} //Check if memory has been allocated
	}
	else {
		memset(*Packet, 0, packetSize);   /// resets buffer to all zeros
	}

	int offset = 0;
	offset = (packetNumber - 1 ) * (packetSize);
	memcpy(*Packet, sourcePackets + offset , packetSize);  // copies the packet into the byte array

}

////////////////////////////////////////////////////////////////
// Function to write the output string to the output file   ////
// It writes the output in a little endian system  			////
// It then winds back the write pointer to the correct 		////
// position if the byte and word boundaries do not match	////
////////////////////////////////////////////////////////////////
void write_to_file(unsigned int o[], FILE * outputFile) {

	// writes all the words of the output string to the file
	uint8_t byte;
	for(int word = 0; word < o_word+1; word++ ){
		byte = (o[word]>> 24) & 0xFF;
		fwrite(&byte, 1, 1, outputFile );				// write to file
		byte = (o[word]>> 16) & 0xFF;
		fwrite(&byte, 1, 1, outputFile );				// write to file
		byte = (o[word]>> 8) & 0xFF;
		fwrite(&byte, 1, 1, outputFile );				// write to file
		byte = o[word] & 0xFF;
		fwrite(&byte, 1, 1, outputFile );				// write to file
	}
	// winds back the write pointer the required number of bytes
	int rest;

	if (o_bit<1) {
		fseek(outputFile, -4, SEEK_CUR);				// wind write pointer back 4 bytes as the new word is still empty
	} else {
		rest = 3-(o_bit-1)/8;
		fseek(outputFile, -1*rest, SEEK_CUR);				// wind write pointer back x bytes
	}

}