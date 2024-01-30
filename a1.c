// a1shell.c
//   Erin Jones
/*run it with 
gcc -o a1 a1.c
a1 a1test.a
lcc a1test.e*/
#include <stdio.h>    // for I/O functions
#include <stdlib.h>   // for exit()
#include <string.h>   // for string functions
#include <ctype.h>    // for isspace(), tolower()
#include <time.h>     // for time functions

//declare function prototype in header to avoid errors
short int mystrcmpi(const char *p, const char *q) ;
short int mystrncmpi(const char *p, const char *q, int n);
int isreg(char *p);
unsigned short getreg(char *p);
unsigned short getadd(char *p);

FILE *infile, *outfile;
short pcoffset9, pcoffset11, imm5, imm9, offset6;
unsigned short symadd[500], macword, dr, sr, sr1, sr2, baser, trapvec, eopcode;
char outfilename[100], linesave[100], buf[100], *symbol[500], *p1, *p2, *cp,
     *mnemonic, *o1, *o2, *o3, *label;
int stsize, linenum, rc, loc_ctr, num;
time_t timer;

// Case insensitive string compare
// Returns 0 if two strings are equal.
short int mystrcmpi(const char* p, const char* q) 
{
   char a, b;
   while (1) 
   {
      a = tolower(*p); b = tolower(*q);
      if (a != b) return a-b;
      if (a == '\0') return 0;
      p++; q++;
   }
}

// Case insensitive string compare
// Compares up to a maximum of n characters from each string.
// Returns 0 if characters compared are equal.
short int mystrncmpi(const char* p, const char* q, int n) 
{
   char a, b;
   int i;
   for (i = 1; i <= n; i++) 
   {
      a = tolower(*p); b = tolower(*q);
      if (a != b) return a-b;
      if (a == '\0') return 0;
      p++; q++;
   }
   return 0;
}

void error(char *p)
{
   // Finished
   // Displays error message p points to, line number in linenum, and line in
   // linesave.

   printf("\nError in line %d. Linesave: %s. Error Message: %s\n", linenum, linesave, &p);
}
int isreg(char *p)
{
   //Finished
   // Returns 1 if p points to a register name.
   // Otherwise, returns 0.   

   /*basically checks if the spot for operand 3 in the file has an r in it, if it is then it's a register and not a raw value for pcoffset or whatever*/
                  //checks if the first character of an operand is r    
   return (strncmp(p, "r", 1) == 0 && isdigit(p[1]) && strlen(p) == 2);                                 
                                                      //checking that the chararray p is only length 2, so it's like r3 instead of 0022 for a pcoffset or imm5
}
unsigned short getreg(char *p)              
{
   // Finished
   // Returns register number of the register whose name p points to.
   // Calls error() if not passed a register name.

   //call error if it isn't a register name
   if(!isreg(p)){
    error("Not a register.");
   }

   /*cast it into unsigned short because you're dealing with unsigned registers 0-9
   p[1]-'0* checks the register number. remember it will be r1-7, so p[0] = 'r' and p[1] = 1-7*/
   return (unsigned short)(p[1]-'0');
}
unsigned short getadd(char *p)
{
   //Finished
   // Returns address of symbol p points to accessed from the symbol table.
   // Calls error() if symbol not in symbol table.
    /*First go through the symbol table to see if it's there*/

   //declare i here to avoid errors with casting something into int when you don't mean to 
   int i;
   for(i=0; i<stsize; i++){
      //check if the current value is equal to the symbol p points to
      if (!mystrcmpi(p, symbol[i])){
         //return the memory address if it is. remember that symbol holds pointers to the labels, and symadd holds the memory addresses of the labels
         return symadd[i];
      }
   }
   //if it goes through the whole array without returning then it's not in the array
   error("Label not in symbol table.");
   return 0;

}
int main(int argc,char* argv[])
{
   if (argc != 2)
   {
      printf("Wrong number of command line arguments\n");
      printf("Usage: a1 <input filename>\n");
      exit(1);
   }

   // display your name, command line args, time
   time(&timer);      // get time
   printf("Erin Jones   %s %s   %s", 
           argv[0], argv[1], asctime(localtime(&timer)));

   infile = fopen(argv[1], "r");
   if (!infile)
   {
      printf("Cannot open input file %s\n", argv[1]);
      exit(1);
   }

   // construct output file name
   strcpy(outfilename, argv[1]);        // copy input file name
   p1 = strrchr(outfilename, '.');      // search for period in extension
   if (p1)                              // name has period
   {
#ifdef _WIN32                           // defined only on Windows systems
      p2 = strrchr(outfilename, '\\' ); // compiled if _WIN32 is defined
#else
      p2 = strrchr(outfilename, '/');   // compiled if _WIN32 not defined
#endif
      // can p2 < p1 in following if statement be eliminated by 
      // using strchr(p1, ...) instead of strrchr(outfilename, ...) 
      // in the preceding assignments to p2?
      if (!p2 || p2 < p1)               // input file name has extension?
         *p1 = '\0';                    // null out extension
   }
   strcat(outfilename, ".e");           // append ".e" extension

   outfile = fopen(outfilename, "wb");
   if (!outfile)
   {
      printf("Cannot open output file %s\n", outfilename);
      exit(1);
   }

   loc_ctr = linenum = 0;       // initialize, not required because global
   fwrite("oC", 2, 1, outfile); // output empty header

   // Pass 1
   printf("Starting Pass 1\n");
   while (fgets(buf, sizeof(buf), infile))
   {
      linenum++;  // update line number
      cp = buf;
      while (isspace(*cp))
         cp++;
      if (*cp == '\0' || *cp ==';')  // if line all blank, go to next line
         continue;
      strcpy(linesave, buf);        // save line for error messages
      if (!isspace(buf[0]))         // line starts with label
      {
         label = strdup(strtok(buf, " \r\n\t:"));
         // Finished: checks for a duplicate label, use strcmp().
         int i;
         for(i=0; i<stsize; i++){
            //check if the current value is equal to the symbol p points to
            if (!mystrcmpi(label, symbol[i])){
               //send an error saying it's a duplicate label
               error("Duplicate label found");
            }
         }

         symbol[stsize] = label;
         symadd[stsize++] = loc_ctr;
         mnemonic = strtok(NULL," \r\n\t:"); // get ptr to mnemonic/directive
         o1 = strtok(NULL, " \r\n\t,");      // get ptr to first operand
      }
      else   // tokenize line with no label
      {
         mnemonic = strtok(buf, " \r\n\t");  // get ptr to mnemonic
         o1 = strtok(NULL, " \r\n\t,");      // get ptr to first operand
      }
      if (mnemonic == NULL)    // check for mnemonic or directive
         continue;
      if (!mystrcmpi(mnemonic, ".zero"))    // case insensitive compare
      {
         if (o1)
            rc = sscanf(o1, "%d", &num);    // get size of block from o1
         else
            error("Missing operand");
         if (rc != 1 || num > (65536 - loc_ctr) || num < 1)
            error("Invalid operand");
         loc_ctr = loc_ctr + num;
      }
      else
         loc_ctr++;
      if (loc_ctr > 65536)
         error("Program too big");
   }

   rewind(infile);

   //main debugging
   int doutCount = 0;

   // Pass 2
   printf("Starting Pass 2\n");
   loc_ctr = linenum = 0;      // reinitialize

   //variable storing line, size of line, file
   while (fgets(buf, sizeof(buf), infile))
   {
      // Finished
      // Discard blank/comment lines, and save buf in linesave as in pass 1
      // Tokenize entire current line.
      // Do not make any new entries into the symbol table

      //Debugging
      printf("%d: ", linenum);

      //buf is the current lin ee of the file
      linenum++;  // update line number

      cp = buf;
      while (isspace(*cp))
         cp++;
      if (*cp == '\0' || *cp ==';')  // if line all blank, go to next line
         continue;
      strcpy(linesave, buf);        // save line for error messages
      if (!isspace(buf[0]))         // line starts with label
      {
         /*strtok works like this:
         string being modified, string containing delims that are basically what you're using to split the string*/
         strtok(buf, "\r\n\t:");    //don't need label on pass 2, splits the string using the : after the label
         mnemonic = strtok(NULL, " \r\n\t:"); //tokenize mnemonic/directive
      }
      else   // tokenize line with no label
      {
         mnemonic = strtok(buf, " \r\n\t"); //tokenize mnemonic/directive
      }
      o1 = strtok(NULL, " \r\n\t"); //tokenize first operand
      o2 = strtok(NULL, " \r\n\t"); //second operand
      o3 = strtok(NULL, " \r\n\t"); //third operand

      //main Debugging (the other ones exploded)
      printf("\nmnemonic: %s\n   o1: %s\n   o2: %s\n   o3: %s\n", mnemonic, o1, o2, o3);
      
      if (mnemonic == NULL)    // check for mnemonic or directive
         continue;

      if (!mystrncmpi(mnemonic, "br", 2))    // case sensitive compares
      {
         /*the way the macword works for these is annoying
         so the branch instructions are br: 0000 code pcoffset9
         so they're setting the code to something by manipulating the whole 4 bit instruction
         for instance, brnz's code is 001 so the whole thing would be 0000 001 0 00000000, or 0200*/
         if (!mystrcmpi(mnemonic, "br" ))
            macword = 0x0e00;
         else
         if (!mystrcmpi(mnemonic, "brz" ))
            macword = 0x0000;
         else
         if (!mystrcmpi(mnemonic, "brnz" ))
            macword = 0x0200;
         else
         if (!mystrcmpi(mnemonic, "brn" ))
            macword = 0x0400;
         else
         if (!mystrcmpi(mnemonic, "brp" ))
            macword = 0x0600;
         else
         if (!mystrcmpi(mnemonic, "brlt" ))
            macword = 0x0800;
         else
         if (!mystrcmpi(mnemonic, "brgt" ))
            macword = 0x0a00;
         else
         if (!mystrcmpi(mnemonic, "brc" ))
            macword = 0x0c00;
         else
            error("Invalid branch mnemonic");

         pcoffset9 = (getadd(o1) - loc_ctr - 1);    // compute pcoffset9
         if (pcoffset9 > 255 || pcoffset9 < -256)
            error("pcoffset9 out of range");
         macword = macword | (pcoffset9 & 0x01ff);  // assemble inst
         fwrite(&macword, 2, 1, outfile);           // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "add" ))
      {
         if (!o3)
            error("Missing operand");
         dr = getreg(o1) << 9;   // get and position dest reg number
         sr1 = getreg(o2) << 6;  // get and position srce reg number
         if (isreg(o3)) // is 3rd operand a reg?
         {
            sr2 = getreg(o3);      // get third reg number
            macword = 0x1000 | dr | sr1 | sr2; // assemble inst
         }
         else
         {
            if (sscanf(o3,"%d", &num) != 1)    // convert imm5 field
               error("Bad imm5");
            if (num > 15 || num < -16)
               error("imm5 out of range");
            macword = 0x1000 | dr | sr1 | 0x0020 | (num & 0x1f); 
            
         }
         //Debugging
         //printf("Add: \ndr = %d\n, sr1 = %d\n, sr2 = %d\n", dr, sr1, sr2);
         

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "ld" ))
      {
         dr = getreg(o1) << 9;// get and position destination reg number
         pcoffset9 = (getadd(o2) - loc_ctr - 1);   //compute pcoffset
         if (pcoffset9 > 255 || pcoffset9 < -256)
            error("pcoffset9 out of range");

         //debugging
         //printf("LD: \ndr = %d\n, pcoffset9 = %d\n", dr, pcoffset9);

         macword = 0x2000 | dr | (pcoffset9 & 0x1ff);   // assemble inst; 1ff is 9 1's to isolate the pcoffset 9 field
         fwrite(&macword, 2, 1, outfile); // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "st" ))
      {
         /*should be similar to the ld instruction, just swap some things*/
         sr = getreg(o1) << 9;// get and position destination reg number

         pcoffset9 = (getadd(o2) - loc_ctr - 1);    // compute pcoffset9
         if (pcoffset9 > 255 || pcoffset9 < -256){
            error("pcoffset9 out of range");
         }
         //debugging
         //printf("ST:\n sr = %d\n,pcoffset9 = %d\n", sr, pcoffset9);

         macword = 0x3000 | sr | (pcoffset9 & 0x1ff);   // assemble inst; 1ff isolates the pcoffset9 field
         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "bl" ))
      {
         //unfinished
         //0100 1 pcoffset11

         pcoffset11 = (getadd(o1) - loc_ctr - 1);
         if (pcoffset11 > 1023 || pcoffset11 < -1024)
            error("pcoffset11 is out of range.");

         //0100 0000 0000 0000 | 1000 0000 0000 | 0xxx xxxx xxxx
         macword = 0x4000 | 0x0800 | (pcoffset11 & 0x7ff);

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "blr" ))
      {
         //finished
         //0100 000 baser offset6

         baser = getreg(o1) << 6;        // get reg number and position it
         if (o2)                         // offset6 specified?
         {
            if (sscanf(o2,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else {
            num = 0;                           // offset6 defaults to 0
         }

         macword = 0x4000 | baser | (num & 0x3f);

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "and" ))
      {
         //finished
         //should be very similar to the add instruction
         //actually it should probably be identical as this program doesn't actually do the operations itself

         if (!o3)
            error("Missing operand");
         dr = getreg(o1) << 9;   // get and position dest reg number
         sr1 = getreg(o2) << 6;  // get and position srce reg number
         if (isreg(o3)) // is 3rd operand a reg?
         {
            sr2 = getreg(o3);      // get third reg number

            //debugging
            //printf("AND:\n dr = %d\n, sr1 = %d\n, sr2 = %d\n", dr, sr1, sr2);

            macword = 0x5000 | dr | sr1 | sr2; // assemble inst: opcode of and is 0101
         }
         else
         {
            if (sscanf(o3,"%d", &num) != 1)    // convert imm5 field
               error("Bad imm5");
            if (num > 15 || num < -16)
               error("imm5 out of range");

            //debugging
            //printf("AND:\n dr = %d\n, sr1 = %d\n, imm5 = %d\n", dr, sr1, num);

            macword = 0x5000 | dr | sr1 | 0x0020 | (num & 0x1f); //isolates the imm5 field but keeps the number written in the assembly code
            /*say the instruction is: add 0, 1, 5
            num will be 5, so to isolate that you do an OR with 0000 0000 0001 1111 to retain the value*/
         }

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "ldr" ))
      {
         //finished

         /*
         Problem: for the output 8 the assembled instruction should be 6104, getting 6100 instead
         problem seems to be with the offset 6 again

         FIXED - same problem as the str instruction, was scanning into the o2 field instead of the o3
         */

         dr = getreg(o1) << 9;   // get and position dest reg number
         baser = getreg(o2) << 6;  // get and position srce reg number

         if (o3)                         // offset6 specified?
         {
            if (sscanf(o3,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else
            num = 0;                           // offset6 defaults to 0

         //debugging
         //printf("LDR:\ndr = %d\nbaser = %d\noffset6 = %d\n", dr, baser, num);

         macword = 0x6000 | dr | baser | (num & 0x3f);
         
         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "str" ))
      {
         //finished

         /*output should be 7040 for -11 and 7041 for -12, getting 707f instead
         707f - 0111 0000 0111 1111
         7040 - 0111 0000 0100 0000
         7041 - 0111 0000 0100 0001
         so it looks like the problem is with the offset6 range
         removing the (num & 0x3f) and just having | (num) makes the trap vector out of range somehow
         FIXED - the sscanf was scanning the offset6 value into the second operand rather than the third, overwritting the base register
         */


         /*should be same as ldr just with a different opcode*/
         sr = getreg(o1) << 9;   // get and position dest reg number
         baser = getreg(o2) << 6;  // get and position srce reg number

         if (o3)                         // offset6 specified?
         {
            if (sscanf(o3,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else
            num = 0;                           // offset6 defaults to 0
         
         //debugging
         //printf("STR:\ndr = %d\nbaser = %d\npcoffset9 = %d\n", dr, baser);

         macword = 0x7000 | sr | baser | (num & 0x3f);   // assemble inst; 3f = 111111

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "not" ))
      {
         //finished
         dr = getreg(o1) << 9;   // get and position dest reg number
         sr1 = getreg(o2) << 6;  // get and position srce reg number

         //debugging
         //printf("NOT:\ndr = %d\nsr1 = %d\n", dr, sr1);

         //1001 xxxy yy00 0000
         macword = 0x9000 | dr | sr1;

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "jmp" ))     // also ret instruction
      {
         baser = getreg(o1) << 6;        // get reg number and position it
         if (o2)                         // offset6 specified?
         {
            if (sscanf(o2,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else
            num = 0;                           // offset6 defaults to 0


         //debugging
         //printf("JMP:\nbaser = %d\noffset6 = %d\n", baser, num);

         // combine opcode, reg number, and offset6
         macword = 0xc000 | baser | (num & 0x3f);       
         fwrite(&macword, 2, 1, outfile);  // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "ret" ))     // also ret instruction
      {
         //unfinished
         // code here is similar to code for jmp except baser
         // is always 7 and optional offset6 is pointed to by
         // o1, not by o2 as in jmp

         baser = 7 << 6; //link register

         if (o1)                         // offset6 specified?
         {
            if (sscanf(o1,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else
            num = 0;                           // offset6 defaults to 0
         //debugging
         //printf("RET:\noffset6 = %d\n", num);

         //1100 0000 0000 0000 | 0001 0000 0000 | 1100 0000 | 0000 0000 00xx xxxx
         macword = 0xc000 | 0x100 |  baser | (num & 0x3f);       
         fwrite(&macword, 2, 1, outfile);  // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "lea" ))
      {
         //unfinished
         //1110 dr pcoffset9
         dr = getreg(o1) << 9;// get and position destination reg number
         pcoffset9 = (getadd(o2) - loc_ctr - 1);
         if (pcoffset9 > 255 || pcoffset9 < -256)
               error("pcoffset9 out of range");

         //debugging
         //printf("LEA:\ndr = %d\npcoffset9 = %d\n", dr, pcoffset9);

         macword = 0xe000 | dr | (pcoffset9 & 0x1ff);   // assemble inst

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      /*for the trap instructions i think it should just be like this, i would do a switch statement but 
      assembly doesn't have anything that's like just a trap instruction, so the mnemonics would just have 
      these mnemomics*/

      else
      if (!mystrcmpi(mnemonic, "halt" ))
      {

         //debugging
         //printf("HALT\n");

         //halt: 1111 0000 0000 0000
         macword = 0xf000;

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "nl" ))
      {
         //nl: 1111 000 0000 00001

         //debugging
         //printf("NL\n");

         /*other instructions with imm5 fields use num & 11111 and the trapvectors are in an imm5 field so we ball*/
         macword = 0xf001;
                 //1111 0000 0000 0000 OR 0000 0000 0000 0001
                 //should get 1111 0000 0000 0001


         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "dout" ))
      {
         //dout : 1111 sr 0000 00010

         if(o1 == NULL || !mystrcmpi(o1, ";")) {
            sr = 0; //sr defaults to 0
         } else {
            sr = getreg(o1) << 9;
         }

         //main debugging
         doutCount++;
         printf("\nDOUT #%d\n\n", doutCount);

         //debugging
         //printf("DOUT: dr = %d\n", dr);

         macword = 0xf000 | sr | 0x0002;
         /* 1111 0000 0000 0000 OR 0000 dr0 0000 0000 OR 0000 0000 0000 0010
         should be 1111 0000 0000 00010*/

         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, ".word"))
      {
         //finished

         /*weird bug:
         one of the .word outputs is getting 0100 instead of 064
         64 in hex is 100 in decimal, so for some reason that specific output is being done in decimal
         It doesn't effect the output, but it is weird
         */

         /*this should just contain a singular number
         &num is the value num is pointing to*/
         //sscanf: data being changed, hex format specifier, where the data is being read from
         sscanf(o1, "%x", &num);
         macword = num;
         fwrite(&macword, 2, 1, outfile);
         loc_ctr++;

      }

      else
      if (!mystrcmpi(mnemonic, ".zero"))
      {
         macword = 0;
         sscanf(o1, "%d", &num);             // get size of block
         loc_ctr = loc_ctr + num;            // adjust loc_ctr

         //debugging
         //printf(".ZERO: \n");
         

         while (num--)                       // write out a block of zeros
            fwrite(&macword, 2, 1, outfile);
      }
      else
         error("Invalid mnemonic or directive");
   }
   // Close files.
   fclose(infile);
   fclose(outfile);
   return 0;
}


/*Output:
1 :) - tests lea, st, ld, and .word
2 :) - tests add v1
3 :) - tests add v2
4 :) - tests and
5 :) - tests br, bp, not, brz, jmp, bl, blr, and, .zero, 
6 :) - tests lea, ldr
7 :) - tests ld, ldr
8 :( - tests lea, ldr, ld, not
-9 :) - ld, not
-10 :) - add, st, ld
-11 :) - add, ld, str, ld
-12 :) - add, lea, str, ld
*/