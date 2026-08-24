

#include "all.h"


/*
Decompress a RLE buffer
length : the length in the output buffer
output : output buffer where to write the deconmpressed data
input : the compressed data itself
*/
void RLEDecompress(unsigned char *output,unsigned char *input,unsigned short length)
{
   signed char count;

   while (length > 0) {
      count = (signed char)*input++;
      if (count > 0) {                      /* replicate run */
         memset(output,*input++,count);   
      } else if (count < 0) {               /* literal run */
         count = (signed char)-count;
         memcpy(output,input,count);
         input += count;
      } /* if */
      output += count;
      length -= count;
   } /* while */
   
}


/*
Encode an RLE buffer
Return the length in the output buffer
output : output buffer where to write the conmpressed data
input : the data to compress
*/
unsigned short RLECompress(unsigned char *output,unsigned char *input,unsigned short length)
{
   unsigned short count = 0,index,i;
   unsigned char token;
   unsigned short out = 0;

   while (count < length) {
      index = count;
      token = input[index++];
      while ( index < length && index - count < 127 && input[index] == token )
         index++;
      if (index - count == 1) {
         /* 
            Failed to "replicate" the current token. See how many to copy.
            Avoid a replicate run of only 2-tokens after a literal run. There
            is no gain in this, and there is a risK of loss if the run after
            the two identical tokens is another literal run. So search for
            3 identical tokens.
         */
         /*while ( ((index < length) && ((index - count) < 127) && ((input[index] != input[index-1])))
               || ((index > 1) && (input[index] != input[index-2])) )
            index++;
           */ 
         while (index < length && index - count < 127 && (input[index] != input[index-1]
         				|| index > 1 && input[index] != input[index-2]))
            index++;
            
         /* 
            Check why this run stopped. If it found two identical tokens, reset
            the index so we can add a run. Do this twice: the previous run
            tried to detect a replicate run of at least 3 tokens. So we may be
            able to back up two tokens if such a replicate run was found.
         */
         while (index < length && input[index] == input[index-1])
            index--;
         output[out++] = (unsigned char)(count - index);
         for (i=count;i<index;i++)
            output[out++] = input[i];
      } else {
         output[out++] = (unsigned char)(index - count);
         output[out++] = token;
      } /* if */
      count=index;
   } /* while */
   return(out);
}