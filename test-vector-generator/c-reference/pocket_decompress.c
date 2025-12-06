/*
 ============================================================================
 Name        : pocket_decompress.c
 Author      : David Evans, ESA/ESOC
 Version     : 1.0
 Copyright   : No copyright notice
 Description : POCKET+ Decompression in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

// FUNCTIONS
int getValueFromCompressedPacket(int numberOfBits, uint8_t compressed[]);		// function to read the next x bits from the compressed packet
void counters(uint8_t compressed[], int flag, uint8_t M[], uint8_t X_pos[], int packetSize); // function to process ht or qt
void bitStuffing(); 	// function to ignore any zeros added to reach a byte boundary
int get_delta(uint8_t compressed[]);// function to read the next counter from the compressed bit string
void pointer_to_dt(uint8_t compressed[]);   // function to bring the read pointer to ut. Only needed the initialisation packet. 
void pointer_to_ut(uint8_t compressed[]);	// function to bring the read pointer to dt. Only needed the initialisation packet. 

// GLOBAL VARIABLES
int currentByte = 0;	// set the read byte pointer for the compressed file
int currentBit = 7;		// set the read bit pointer for the compressed file

int rt;								// define the uncompressed flag - called rt in the standard
int Vt = 0;							// defines the robustness level - Vt in the standard 
float take_away;					// variable to take care of bit lengths not divisible by 8
int ct = 0;							// defines if pt is set to one two times in the t-Vt period - ct in the standard


///////////////////////
//       MAIN       ///
///////////////////////
int main(int argc, char** argv)  {

	if(argc != 2){
		printf("%s\n", "Invalid input arguments." );
		printf("%s\n", "Use: pocket_decompress.exe <compressed filename> " );
		printf("%s\n", "compressed file should be in this directory" );
		printf("%s\n", "if successfull the decompressed file will have the same name with a *.depkt extension, in this directory" );
		return 1;
	}

	//////////////////
	// Declarations //
	//////////////////
	char inputFileName0[64] = {0};	// define the compressed file name
	long compressed_file_length;	// defines the size of the compressed file
	int packetSize;				// size of uncompressed packet in bytes
	int next_bit;				// generic variable for reading values from the compressed file
	int byte;					// general variable for a byte in a vector
	uint8_t *compressed;		// byte array from reading compressed file
	int M_bit;					// define a variable for the value of a mask bit
	int NP_bit;					// define a variable for the value of an unpredictable bit
	int I_bit;					// define a variable for the value of the uncompressed bit
	int X_bit;					// define a variable for the value of the change bit
	int dt;						// defines dt i.e. both rt and ft are set to zero, dt in the standard
	int ft;						// defines if an entire new mask will be sent - called ft in the standard
	float d;                  	// generic variable to take care of bit lengths not divisible by 8	


	//////////////////////
	// Input the compressed file i.e. compressed.bin and load into compressed byte array
	//////////////////////
	strcpy(inputFileName0, argv[1]); 	// set first input parameter to the original file name
	printf("%s %s \n", "compressed filename:", inputFileName0 );

	//////////////////////
	// Input the compressed file into compressed byte array
	//////////////////////
	FILE *fileptr;
	fileptr = fopen(inputFileName0, "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          	// Jump to the end of the file
	compressed_file_length = ftell(fileptr);             	// Get the current byte offset in the file
	rewind(fileptr);                      	// Jump back to the beginning of the file

	compressed = (uint8_t *)malloc((compressed_file_length+1)*sizeof(uint8_t)); // Enough memory for file
	fread(compressed, compressed_file_length, 1, fileptr); // Read in the entire file into compressed byte array
	fclose(fileptr); // Close the file

	printf("%s %ld \n", "compressed_file_length", compressed_file_length );


	//////////////////////
	// CREATES THE OUTPUT FILE : *.DEPKT
	//////////////////////
	FILE *outputFile;
	char outfile[50];
	strcpy (outfile, inputFileName0);
	strcat(outfile, ".depkt");
	outputFile = fopen(outfile, "wb");
	printf("%s %s \n", "outfile", outfile );


	///////////////////////////////////////////////////////////////////
	// The next section is all about getting the packet length before starting to process the file.
	// Hence we have to run through the entire header until we get to COUNT(F) in ut, EQ 22 case 3
	// Once we have the packet length we can size the buffers correctly, then we process the file
	//////////////////////////////////////////////////////////////////
	currentByte = 0;	// jumps to the first byte of the compressed packet file
	currentBit = 7;		// jumps to the first bit of the compressed packet file


	next_bit = getValueFromCompressedPacket(2, compressed);	// checks if the first two bits are 10, if so there is no counter and we just processed to reading Vt
	if (next_bit==2)	{									// No counters.
		Vt = getValueFromCompressedPacket(4, compressed);	// read Vt
	} else {
		currentBit = 7;										// set bit position back to start of file
		currentByte = 0;									// set byte position back to start of file
		pointer_to_dt(compressed);  						// runs through the counters until it finds the start of dt
	}

	// read through various fields
	next_bit = getValueFromCompressedPacket(1,compressed);	// read dt, needs to be zero for initialisation 

	next_bit = getValueFromCompressedPacket(1, compressed);	// read ft, needs to be one for initialisation

	pointer_to_ut(compressed); 	// Call function to find the end of qt

	next_bit = getValueFromCompressedPacket(1, compressed);	// read the bit before the COUNT(F), so we read the first bit from EQ 22 case 3  

	// We are at COUNT(F) so read it
	next_bit = get_delta(compressed);  // reads the packet length in bits

    /////////////////////////////////////
    // Deals with the case that the bit_length is not divisble by 8 i.e. need to bring it to the byte boundary and sets PacketSize
	// The take_away variable records the amount of extra bits to make it to the next byte boundary
    ////////////////////////////////////////
	d = (float) next_bit;
	packetSize=next_bit/8;	// converts to bytes
	d = d/8;
	take_away = (d-packetSize)*8;
	take_away = 8-take_away;
	if (take_away==8) {take_away=0;}
	if (d-packetSize>0) {packetSize++;}


   // reset read counters
	currentByte = 0;	// jumps to the first byte of the compressed packet file
	currentBit = 7;		// jumps to the first bit of the compressed packet file


	///////////////////////////////////////////////////////////////
	// Now we have packetSize we can initialise the arrays I and M and X_pos to that packet size  
	////////////////////////////////////////////////////////////////
	uint8_t I[packetSize];			// define a bit vector for the de-compressed packet
	uint8_t M[packetSize];			// define a bit vector for the decompression mask - which reflects the mask used to compress the incoming packet
	uint8_t X_pos[packetSize];		// define a bit vector to record only positive changes in Xt for the present packet
	memset(M, 0, packetSize);		// initially set the mask to all zeros


	//////////////////
	/// MAIN LOOP
	//////////////////
	do {
  
		ct = 0;       //reinialises ct to zero each cycle

		/////////////////////////////////
		// DEAL WITH MASK CHANGES (ht)
		/////////////////////////////////
		next_bit = getValueFromCompressedPacket(2, compressed);	 // Read the first two bits of the compressed packet

		if (next_bit==2)	{									// checks if the first two bits are 10, so no changes to the mask and we can proceed directly to reading Vt
			Vt = getValueFromCompressedPacket(4, compressed);	// read Vt										
		} else {
			currentBit = 7;										// set bit position back to start of the bit vector as we are going to process ht
			counters(compressed, 1, M, X_pos, packetSize);  	// Process ht, update the mask and read Vt
		}

		dt = getValueFromCompressedPacket(1, compressed);	// read dt and set rt and ft to zero if it is one (EQ 13)
		if (dt == 1) {
			rt = 0;			
			ft = 0;
		} 


		/////////////////////////////////
		// DEAL WITH MASK REPLACEMENT (qt)
		/////////////////////////////////
		if (dt == 0) { 					 // if dt is zero then have to consider a possible entire mask has been requested by setiing ft to one

			ft = getValueFromCompressedPacket(1, compressed);	// read ft

			if (ft==1)	{ 								// if ft=1, then qt exists and we have a new complete mask to read and load	

				counters(compressed, 3, M, X_pos, packetSize);  // Process qt (setting flag=3)

			}
		}

		///////////////////////////////////////////////////////////////////////////
		// DEALS WITH NON PREDICTABLE BIT VALUES AND PACKETS SENT IN THE CLEAR (ut)
		///////////////////////////////////////////////////////////////////////////
		if (dt==0)  {												// if dt is zero then we cannot assume that it is only NP bits that follow
			rt = getValueFromCompressedPacket(1, compressed);		// read rt

			/////////////////////////////////////////////
			// DEALS WITH PACKETS SENT IN THE CLEAR (ut)
			/////////////////////////////////////////////
			if (rt==1)	{											// if rt is one then the next bit vector will be COUNT(F) in bits

				/////////////////////////////////////////////////////////
				// Deals with reading the packet length field (COUNT(F)
				/////////////////////////////////////////////////////////
				next_bit = get_delta(compressed);					// read packet length in bits                   

   				 /////////////////////////////////////
   				 // Deals with the case that the bit_length is not divisble by 8 i.e. need to bring it to the byte boundary and sets PacketSize
				// The take_away variable records the amount of extra bits to make it to the next byte boundary
    			////////////////////////////////////////
				d = (float) next_bit;
				packetSize=next_bit/8;	// converts to bytes
				d = d/8;
				take_away = (d-packetSize)*8;
				take_away = 8-take_away;
				if (take_away==8) {take_away=0;}
				if (d-packetSize>0) {packetSize++;}

			}

		}

			//////////////////////////////////////////////////////////////////////
			// Copies the packet sent in the clear into the output bit vector I
			//////////////////////////////////////////////////////////////////////
			if (rt){																	// if rt=1 then NP string contains the entire uncompressed packet

				for(int currentByte1 = 0; currentByte1 < packetSize; currentByte1++ ){	// cycle through the bytes
					byte = getValueFromCompressedPacket(8, compressed);					// read the next byte
					I[currentByte1] = byte;												// write whole byte to output I array
				}

				////////////////////////////////////////////////////////////////////
				// Deals with the case the bit length is not a multipe of a byte
				// clears the bit values going backwards from the LSB to MSB and updates the write pointers
				///////////////////////////////////////////////////////////////////
				for(int bit = 0; bit < take_away; bit++){				            // sets up bit loop
						I[packetSize-1] &= ~(1 << bit);				               // clear the bit
						currentBit++;
						if (currentBit > 7) {
							currentBit=0;
							currentByte--;
						}
				}


			} else {			// NP string contains only the values of the bits that are only masked and if ct=1 then the bit values of all the positive delta changes in the mask

				///////////////////////////////////////////////////////
				// DEALS WITH UNPREDICTABLE BIT FIELD (ut) WHEN ct= 0
				///////////////////////////////////////////////////////
				if (ct==0){ // If ct=0 then NP bit string is only the masked NP bits

					for(int byte = packetSize-1; byte > -1; byte-- ){						// cycle through the bytes
						for (int current_mask = 0; current_mask < 8; current_mask++) {		// cycle through the bits
							M_bit = M[byte] & (1 << current_mask);							// get current bit value for the mask 
							if (M_bit) {													// if a mask bit is set
								NP_bit = getValueFromCompressedPacket(1, compressed);  		// read the value of the next NP bit 
								I_bit = I[byte] & (1 << current_mask);						// check to see if current bit value of I at that position is one
								if (I_bit) {I_bit=1;}										// if it is one, then set I_bit to one, so we can compare it
								if (I_bit!=NP_bit) {										// Are I_bit and NP_bit the same?
									I[byte] ^= 1 << current_mask;             				// If NP bit and I bit are not the same then toggle I bit
								}
							}
						}
					}



				} else { 		// If ct=1 then we have to OR the mask with X to get both the contents of NP bits and the values of the positive delta changes to the mask

				///////////////////////////////////////////////////////
				// DEALS WITH UNPREDICTABLE BIT FIELD (ut) WHEN ct= 1
				///////////////////////////////////////////////////////
					for(int byte = packetSize-1; byte > -1; byte-- ){					// cycle through the bytes
						for (int current_mask = 0; current_mask < 8; current_mask++) {	// cycle through the bits
							M_bit = M[byte] & (1 << current_mask);						// get current bit value for the mask 
							X_bit = X_pos[byte] & (1 << current_mask);					// get current bit value for that position for any positive changes to the mask // DIFFERENT FROM ABOVE
							if ((M_bit) || (X_bit))  {									// if either the mask bit is set or the positive chnage bit is set 				// DIFFERENT FROM ABOVE
								NP_bit = getValueFromCompressedPacket(1, compressed);  	// read the value of the next NP bit  
								I_bit = I[byte] & (1 << current_mask);					// check to see if current bit value of I at that position is one
								if (I_bit) {I_bit=1;}									// if it is one, then set I_bit to one, so we can compare it
								if (I_bit!=NP_bit) {									// Are I_bit and NP_bit the same?
									I[byte] ^= 1 << current_mask;             			// If NP bit and I bit are not the same then toggle the bit in I
								}
							}
						}
					}
				}


			}

    	//////////////////////////////////////////////
    	// WRITE THE UNCOMPRESSED BIT VECTOR TO FILE
    	/////////////////////////////////////////////
		fwrite(&I, 1, packetSize, outputFile );				// writes the uncompressed packet to the output file
		bitStuffing(); 	// ensure that any trailing zeros are ignored

	} while (currentByte<compressed_file_length);
    /////////////////////////////////////
    // END OF FILE PROCESSING LOOP
    /////////////////////////////////////

	/// Close the uncompressed file
	fclose(outputFile); 

	// Signal completion
printf("%s","Success");

}


////////////////////////////////////////////////////////
//         END OF MAIN								////
////////////////////////////////////////////////////////






/////////////////////////////////////////////////
// FUNCTION TO RETURN A VALUE OF THE NEXT X BITS FROM THE COMPRESSED BYTE ARRAY
/////////////////////////////////////////////////
int getValueFromCompressedPacket(int numberOfBits, uint8_t compressed[]){
	uint32_t value = 0; //variable to store the value
	numberOfBits -= 1; //because here zero is included.
	/* loop which gets <numberOfBits> bits and store them in <value>. It goes from left to right. */

	for (int bitCounter = numberOfBits; bitCounter >= 0; bitCounter --){
		value |= ( ((compressed[currentByte] >> currentBit) & 1) << bitCounter ); // get the value
		currentBit -= 1; // move to the next bit (go right)
		if (currentBit <0){currentBit = 7; currentByte += 1;} // we are already in the next register, reset and go into the next register
	}
	return value;
}




/////////////////////////////////////////////////
// FUNCTION TO JUMP TO THE NEXT BYTE BOUNDARY
/////////////////////////////////////////////////
void bitStuffing() {

	if (rt==1) {
		if ((currentBit+take_away) < 7) {	// take away the extra bits first to see if you need to skip a byte
			currentByte++;			// and move to the next byte
		}
	} else {
		if ((currentBit) < 7) {	// take away the extra bits first to see if you need to skip a byte
			currentByte++;			// and move to the next byte
		}
	}
	currentBit = 7;			// set the current bit read position to seven i.e. start position
}


/////////////////////////////////////////////////
// Only function to get the read pointer to dt in the ht vector
// Only needed in the initialisation packet
////////////////////////////////////////////////
void pointer_to_dt(uint8_t compressed[]) {

	int et;								
	int count_number=0;						// stores number of counts in the compressed mask

	// run through the counters
	int d_count=1;							// initialises loop
	while (d_count) {						// continue through the counter sequence until the counter equals zero
		d_count = get_delta(compressed);
		if (d_count>0) {count_number++;}
	}

	// read Vt
	Vt = getValueFromCompressedPacket(4, compressed);	// read Vt

	// read et, kt and ct
	if (Vt>0) {
  		et = getValueFromCompressedPacket(1, compressed);	// read et

		if (et>0) {
			for(int a = 0; a < count_number; a++ ){	// cycle through the mask updates and record them
				et = getValueFromCompressedPacket(1, compressed);	// get next bit value
			}
			et = getValueFromCompressedPacket(1, compressed);	// read ct, just says et here to reuse the variable
  		  }
	}

}


/////////////////////////////////////////////////
// Only function to get the read pointer to ut in the bit vector
// Only needed in the initialisation packet
//////////////////////////////////////////////////////////
void pointer_to_ut(uint8_t compressed[]) {
	int d_count=1;							// initialises the following loop
	
	// Read the counts until a zero is returned meaning that's the end
	while (d_count) {						// continue through the counter sequence until the counter equals zero
		d_count = get_delta(compressed);
	}
}


///////////////////////////////////////////////////////////
// Function to process ht (if flag=1) or qt (if flag=3) ///
///////////////////////////////////////////////////////////
void counters(uint8_t compressed[], int flag, uint8_t M[], uint8_t X_pos[], int packetSize) {
	int d_count=1;							// defines the delta count read from the compressed packet
	int bit_position = packetSize*8; 		// Sets bit position to end of packet
	int Bit_start_of_counters;				// read position save variable
	int Byte_start_of_counters;				// read position save variable
	int Bit_after_ct;						// read position save variable
	int Byte_after_ct;						// read position save variable
	int flag_all_negs = 0;					// flag for: kt does not exist, et is 0 and yt is all negative changes (EQ 18, case 2, EQ 19, case 1, EQ 20 case 1)
	int flag_pos_rt_is_zero = 0;			// flag for: et and kt do not exist, (EQ 18 case 1, EQ 19, case 1, EQ 20 case 1)
	int flag_pos_rt_is_not_zero = 0;		// flag for: kt = yt and et is one, (EQ 18 case 3, EQ 19, case 2, EQ 20 case 1)

	int yt[packetSize*8];					// yt array: for storing types of mask updates (positive or negative)
	int count_number=0;						// stores number of changes in mask, so it knows how long yt will be
	int et=0;								// stores et value from standard


    //////////////////////////////////////////////////////////////////////////////////////
	/// IF IT IS A MASK REPLACEMENT THEN SET THE MASK TO ALL ZERO SO IT CAN BE OVERWRITTEN
    ///////////////////////////////////////////////////////////////////////////////////////
	if (flag==3) {							// if its a mask replacement

		memset(M, 0, packetSize);			// reset mask to zero

	} else {

    	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// This is a delta mask process run therefore we need to work out what et, kt and ct are. Only then can we process the delta mask in the second run
    	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		Byte_start_of_counters = currentByte;	// store the byte read value, start of counts
		Bit_start_of_counters = currentBit;		// store the bit read value, start of counts

		/////////////////////////////// 
		// Read the counts until a zero "10" is found. Record how many counts there in count_number
        /////////////////////////////////
		while (d_count) {						// continue through the counter sequence until the counter equals zero
			d_count = get_delta(compressed);   // Get the counter from the compressed packet
			if (d_count>0) {count_number++;}   // If it is not a zero ("10") then increment the variable recording the number of counts 
		}

		Vt = getValueFromCompressedPacket(4, compressed);	// read Vt
	
        /////////////////////////////
		// READ et, yt, kt and ct 
        //////////////////////////////
		if (Vt==0) 	{
			flag_pos_rt_is_zero=1;  //As Vt is zero, et and kt do not exist, (EQ 18 case 1, EQ 19, case 1, EQ 20 case 1)
	  	  } else {
	  		et = getValueFromCompressedPacket(1, compressed);	// read et
	  		  if (et==0) {
	  			flag_all_negs = 1;  // kt does not exist, et is 0 and yt is all negative changes (EQ 18, case 2, EQ 19, case 1, EQ 20 case 1)
	  		  } else {
	  			flag_pos_rt_is_not_zero = 1;  // kt = yt and et is one, (EQ 18 case 3, EQ 19, case 2, EQ 20 case 1)
				for(int y_index = 0; y_index < count_number; y_index++ ){	// cycle through the mask updates and record them NOTE THE INDEX IS INCREASING AS yt IS WRITTEN FORWARDS 
					yt[y_index] = getValueFromCompressedPacket(1, compressed);	// read next bit value into yt
				}
				ct = getValueFromCompressedPacket(1, compressed);	// read ct (EQ 18, case 1, EQ 19, case 2, EQ 20 case 2 or 3)
	  		  }
		}

		Byte_after_ct = currentByte;  // store current byte read values i.e. the end of the flags after ct
		Bit_after_ct = currentBit;    // store current bit read values i.e. the end of the flags after ct

		currentByte = Byte_start_of_counters;	// set byte read value to the start of the counts
		currentBit = Bit_start_of_counters;      // set bit read value to the start of the counts
	}

	////////////////////////////////////////////////////////////////////////////////////////////
    // RUN THROUGH THE COUNTERS AGAIN AND PROCESSES THEM ACCORDING TO THE FLAGS DETERMINED ABOVE
    ////////////////////////////////////////////////////////////////////////////////////////////
	if (flag_pos_rt_is_not_zero==1) {
		memset(X_pos, 0, packetSize);	// sets pos only changes mask back to all zero as we need it later because yt exists
	}
	
	int y_index = count_number;    				// this is used as an index in yt[y_index] array. NOTE WE SET IT TO MAX AND THEN DECREASE AS yt IS WRITTEN FORWARDS BUT WE ARE PROCESSING BACKWARDS 
 	bit_position=bit_position-take_away;		// deals with a mask lengths that do not start on the byte boudary

    d_count=1;                              // just to intialise the loop below 
	while (d_count) {						// continue through the counter sequence until the counter equals zero

		d_count = get_delta(compressed);		// Get the counter from the compressed packet

		if (d_count>0) { 									// do this only if the delta count was not zero
			bit_position=bit_position-d_count; 				// calculate the new bit position by decreasing the 

			int byte=bit_position/8;						// calculate the new byte position
			int move=7-(bit_position-(bit_position/8)*8);	// calculates the number of left shifts it needs to create a mask with just that correct bit set

			if (flag_all_negs==1) {					// if only neg mask updates in Xt then
				M[byte] |= 1<<move;					// set that bit in the mask
			}

			if (flag_pos_rt_is_zero==1) {			// As Vt is zero, we can simply invert the mask value when we see a change
				M[byte] ^= (1 << move);				// toggles mask bit
			}

			if (flag_pos_rt_is_not_zero==1) {		// There are neg and pos updates in Xt and Vt>0, hence we read yt (= kt) to see how to update the mask either positively or negatively
				y_index--;							// INDEX DECREASES AS yt IS WRITTEN FORWARDS BUT WE ARE PROCESSING BACKWARDS 
				if (yt[y_index]==0)	{				// if the yt entry is a zero, we should set the mask bit as it is a negative change
					M[byte] |= 1<<move;	            // set bit in mask 
				} else {							// if the yt entry is a one, we should unset the mask bit as it is a positive change
					M[byte] &= ~(1 << move);		// then clear the bit in the mask
					X_pos[byte] |= 1<<move;	        // but set the bit in X_pos which is recording the positions of positive changes in the mask
				}
			}

			if (flag==3) {							// if a complete mask update flag is set, then none of the other cases are activated, only this one
				M[byte] |= 1<<move; 				// set the bit in the absolute mask, note this only gives us the HXOR result of the new mask - it is processed further in a few steps
			}
		}
	}

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
	// If it's not a mask replacement then the read pointers need to jump over the counters before returning
    // If its a mask replacement then the read position is fine i.e. at the end of qt
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (flag!=3)  {
		currentByte = Byte_after_ct;	
		currentBit = Bit_after_ct;
	}

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// EXTRA PROCESSING IS REQUIRED IN THE CASE OF A MASK REPLACEMENT TO REVERSE THE HORIZONAL XOR PERFORMED BY THE COMPRESSOR
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (flag==3) {  		 // if  new mask update flag set
		// DETERMINE THE VALUE OF THE LSB bit of the LSB byte
		int start=0;		// defines the bit value to use for the mask and initially sets it to zero
		if (M[(packetSize-1)] & 1) { start = 1;} // inverts starting bit value to one if the LSB bit is a one


		// Processes the first byte bit by bit for 7 bits
		for(int bit = 1; bit < 8; bit++){					// sets up bit loop
			if (M[packetSize-1] &  (1<<bit)) {				// checks if the mask bit is set at this position
				if (start) {								// if it is set, then it signals a change in value, changes the bit value to set
					start=0;								// if it was a one, set it to zero
				} else {									// otherwise
					start=1;								// if it was a zero, set it to one
				}
			}
			if (start) {									// If the bit value to set is a one
				M[packetSize-1] |= 1<<bit;					// sets the bit in the mask to one
			} else {										// otherwise
				M[packetSize-1] &= ~(1 << bit);				// clears the bit in the mask
			}
		}

		// deal with the rest of the bytes, each for 8 bits
		for(int byte = packetSize-2; byte >= 0; byte--){	// sets up byte loop
			for(int bit = 0; bit < 8; bit++){				// sets up bit loop
				if (M[byte] &  (1<<bit)) {					// checks if the mask bit is set at this position
					if (start) {							// if it is set, then it signals a change in value, changes the bit value to set
						start=0;							// if it was a one, set it to zero
					} else {								// otherwise
						start=1;							// if it was a zero, set it to one
					}

				}
				if (start) {							// If the bit value to set is a one
					M[byte] |= 1<<bit;					// sets the bit in the mask to one
				} else {								// otherwise
					M[byte] &= ~(1 << bit);				// clear the bit in the mask
				}
			}
		}
	}

}





/////////////////////////////////////////////////
// FUNCTION TO READ COUNT AND GET DELTA
/////////////////////////////////////////////////
int get_delta(uint8_t compressed[]) {

	int next_bit;	// generic variable to capture next bits to read from the compressed packet
	int d_count;    // this is the delta value to return

	next_bit = getValueFromCompressedPacket(1, compressed);	// get next bit value
	if (next_bit==0)	{		// a zero is bit number one, COUNT = 1
		d_count = 1;				// the count is one
	} else {					// a one is bit number one
		next_bit = getValueFromCompressedPacket(1, compressed);	// get next bit value
		if (next_bit==0)	{	// a zero is bit number two, COUNT = 0
			d_count = 0;			// this is the end of the sequence of counters "10", count is set to zero to signal it
		} else {				// a one is bit number two
			next_bit = getValueFromCompressedPacket(1, compressed);	// get next bit value
			if (next_bit==0)	{	// a zero is bit number three, COUNT < 32
				d_count = getValueFromCompressedPacket(5, compressed);	// get the value of the next 5 bits
				d_count = d_count+2;	// the count is this value plus two as 1 and 0 are covered
			} else {				// COUNT > 33 so we have three 1's now we have to work out the size of the value field
				//// DETERMINES THE SIZE OF THE COUNTER FIELD TO READ
				int temp=1;			// temporary variable to signal a one has been detected
				int size=0;        	// variable to record the size of counter field
				while (temp) {		// while no one appears
					next_bit = getValueFromCompressedPacket(1, compressed);	// get next bit value
					if (next_bit>0) {temp=0;} // if the next bit is a one, then set temp to zero which will exit the loop the next time
					size++; // increase the size of the counter field to be read
				};
				size=size+5; // increase the size of the value field to be read by 5 as per the standard
				//// RESETS THE READ POSITION
				currentBit += 1; // move back one bit as the one is included in the counter field to read
				if (currentBit >7){currentBit = 0; currentByte -= 1;} // go back a register
				/// READS THE COUNTER FIELD AND ADDS TWO (AS ONE AND ZERO ARE ALREADY USED)
				d_count = getValueFromCompressedPacket(size, compressed);	// reads the counter field
				d_count = d_count+2;	// adds two
			}
		}
	}

	return d_count;   // return the delta calculated
}
