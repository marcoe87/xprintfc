/**
 * @file        xformatc.c
 *
 * @brief       Printf C implementation.
 * 
 * @author      Mario Viara
 * 
 * @version     1.04
 * 
 * @copyright   Copyright Mario Viara 2014  - License Open Source (LGPL)
 * This is a free software and is opened for education, research and commercial
 * developments under license policy of following terms:
 * - This is a free software and there is NO WARRANTY.
 * - No restriction on use. You can use, modify and redistribute it for personal,
 *   non-profit or commercial product UNDER YOUR RESPONSIBILITY.
 * - Redistributions of source code must retain the above copyright notice.
 *
 */

#include  "xformatc.h"

/**
 * Detect support for va_copy
 */
#ifdef	__GNUC__
#define	VA_COPY	1
#endif

#ifndef	VA_COPY
#define	VA_COPY	0
#endif

/**
 * Define XCFG_FORMAT_FLOAT=0 to avoid floating point support
 */
#ifndef XCFG_FORMAT_FLOAT
#define XCFG_FORMAT_FLOAT    1
#endif

enum 
{
    /* The argument is long integer */
    FLAG_LONG       = 0x00000001,
    /* The field is filled with '0' */
    FLAG_ZERO       = 0x00000004,
    /* The field is filled with ' ' */
    FLAG_SPACE      = 0x00000008,
    /* Left alignment */
    FLAG_LEFT       = 0x00000010,
    /* Decimal field */
    FLAG_DECIMAL    = 0x00000020,
    /* Integer field */
    FLAG_INTEGER    = 0x00000040,
    /* Output in upper case letter */
    FLAG_UPPER      = 0x00000080,
    /* Field is negative */
    FLAG_MINUS      = 0x00000100,
    /* Precision set */
    FLAG_PREC       = 0x00000200,
    /* Value set */
    FLAG_VALUE      = 0x00000400,
    /* Buffer set */
    FLAG_BUFFER     = 0x00000800
};

/**
 * Enum for the internal state machine
 */ 
enum State
{
    /* Normal state outputting literal */
    ST_NORMAL = 0,  
    /* Just read % */
    ST_PERCENT = 1, 
    /* Just read flag */
    ST_FLAG = 2,    
    /* Just read the width */
    ST_WIDTH = 3,   
    /* Just read '.' */
    ST_DOT= 4,  
    /* Just read the precision */
    ST_PRECIS = 5,  
    /* Just  read the size */
    ST_SIZE = 6,    
    /* Just read the type specifier */
    ST_TYPE = 7 
};

/**
 * Enum for char class
 */ 
enum CharClass
{
    /* Other char */
    CH_OTHER = 0,
    /* The % char */
    CH_PERCENT = 1,
    /* The . char */
    CH_DOT = 2,
    /* The * char */
    CH_STAR = 3,
    /* The 0 char */
    CH_ZERO = 4,
    /* Digit 0-9 */
    CH_DIGIT = 5,
    /* Flag chars */
    CH_FLAG = 6,
    /* Size chars */
    CH_SIZE = 7,
    /* Type chars */
    CH_TYPE = 8
};


/**
 * Define XCFG_FORMAT_MAKE to build the states[] table.
*/
#ifdef  XCFG_FORMAT_MAKE
#include <stdio.h>
static const unsigned states[] =
{
/* CHAR         NORMAL  PERCENT  FLAG   WIDTH     DOT  PRECIS    SIZE    TYPE*/
/* OTHER    */      0,      0,      0,      0,      0,      0,      0,      0,
/* PERCENT  */      1,      0,      0,      0,      0,      0,      0,      1,
/* DOT      */      0,      4,      4,      4,      4,      0,      0,      0,
/* STAR     */      0,      3,      3,      0,      5,      6,      0,      0,
/* ZERO     */      0,      2,      2,      3,      5,      5,      0,      0,
/* DIGIT    */      0,      3,      3,      3,      5,      5,      0,      0,
/* FLAG     */      0,      2,      2,      2,      2,      2,      2,      0,
/* SIZE     */      0,      6,      6,      6,      6,      6,      6,      0,
/* TYPE     */      0,      7,      7,      7,      7,      7,      7,      0,
};

static unsigned table['z' - ' ' + 1];
#define N (sizeof(table)/sizeof(unsigned))


void make()
{
    int c,i;
    int cl;

    for (i = 0;  i < N ; i++)
    {
        c = ' ' + i;
        switch (c)
        {
            case    '.':
                cl = CH_DOT;
                break;
                
            case    'l':
            case    'h':
                cl = CH_SIZE;
                break;
            case    ' ':
            case    '-':
                cl = CH_FLAG;
                break;
            case    '0':
                cl = CH_ZERO;
                break;
            case    '1':
            case    '2':
            case    '3':
            case    '4':
            case    '5':
            case    '6':
            case    '7':
            case    '8':
            case    '9':
                cl = CH_DIGIT;
                break;
            case    '%':
                cl = CH_PERCENT;
                break;
            case    '*':
                cl = CH_STAR;
                break;

            case    'd':
            case    'i':
            case    'S':
            case    's':
            case    'b':
            case    'x':
            case    'X':
            case    'o':
            case    'u':
            case    'c':
            case    'C':
            case    'p':
            case    'P':
            case    'f':
            case	'B':
                cl = CH_TYPE;
                break;

            default:
                cl = CH_OTHER;
                break;
        }

        table[i] = cl;

        if (i < sizeof(states)/sizeof(unsigned))
            table[i] = table[i] | (states[i] << 4);
    }

    printf("static const unsigned char formatStates[] =\n{\n");

    for (i = 0;  i < N ; i++)
    {
        if (i % 8 == 0)
            printf("\t");
        printf("0x%02X",table[i] & 0xff);
        if (i + 1 < N)
            printf(",");
        if ((i+1) % 8 == 0)
            printf("\n");
    }
    printf("\n};\n\n");
}


int main(int argc,char **argv)
{
    make();
}

#else


/**
 * Used to convert integer
 */
static const char ms_digits[] = "0123456789abcdef";

/**
 * String used when %s as null parameter
 */
static char  ms_null[] = "(null)";
/*
 * String for true value
 */
static char  ms_true[] = "True";

/**
 * String for false value
 */
static char  ms_false[]= "False";


/*
 * This table contains the next state for all char and it will be
 * generated compiling this file with the option -DXCFG_FORMAT_MAKE
 */

static const unsigned char formatStates[] =
{
	0x06,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
	0x10,0x00,0x03,0x00,0x00,0x06,0x02,0x10,
	0x04,0x45,0x45,0x45,0x45,0x05,0x05,0x05,
	0x05,0x35,0x30,0x00,0x50,0x60,0x00,0x00,
	0x00,0x20,0x28,0x38,0x50,0x50,0x00,0x00,
	0x00,0x30,0x30,0x30,0x50,0x50,0x00,0x00,
	0x08,0x20,0x20,0x28,0x20,0x20,0x20,0x00,
	0x08,0x60,0x60,0x60,0x60,0x60,0x60,0x00,
	0x00,0x70,0x78,0x78,0x78,0x70,0x78,0x00,
	0x07,0x08,0x00,0x00,0x07,0x00,0x00,0x08,
	0x08,0x00,0x00,0x08,0x00,0x08,0x00,0x00,
	0x08,0x00,0x00
};



/**
 * Printf like using variable arguments.
 * 
 * @param outchar - Pointer to the function to output one new char.
 * @param arg   - Argument for the output function.
 * @param fmt   - Format options for the list of parameters.
 * @param ...   - Arguments
 *
 * @return The number of char emitted.
 * 
 * @see xvformat
 */
unsigned xformat(void (*outchar)(void *,char),void *arg,const char * fmt,...)
{
    va_list list;
    unsigned count;
    
    va_start(list,fmt);
    count = xvformat(outchar,arg,fmt,list);
    va_end(list);
    (void)list;
    
    return count;
}

/**
 * Internal function to convert a char in upper case.
 *
 * @param c - A char
 * @return The upper case if was in lower case.
 */
static char toUpperCase(char c)
{
    return (char)(c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c);
}

/**
 * Safe version of strlen() if a null pointer is passed return 0.
 *
 * @param s - C  string
 * @return The length of the string
*/
static unsigned xstrlen(const char *s)
{
    unsigned count = 0;
    
    if (s != 0)
    {
        while (*s)
        {
            s++;
            count++;
        }
    }

    return count;
}

static unsigned outBuffer(void (*myoutchar)(void *arg,char),void *arg,const char *buffer,int len,unsigned flags)
{
    unsigned count = 0;
    int i;
    
    for (i = 0; i < len ; i++)
    {
        if (flags  & FLAG_UPPER)
            (*myoutchar)(arg,toUpperCase(buffer[i]));
        else
            (*myoutchar)(arg,buffer[i]);
        count++;
    }

    return count;
}


static unsigned outChars(void (*myoutchar)(void *arg,char),void *arg,char ch,int len)
{
    unsigned count= 0;

    while (len-- > 0)
    {
        (*myoutchar)(arg,ch);
        count++;
    }

    return count;
}


/*
 * Lint want declare list as const but list is an obscured pointer so
 * the warning is disabled.
 */
/*lint -save -e818 */

/**
 * Printf like format function.
 *
 * General format :
 * 
 * %[width][.precision][flags]type
 *
 * - width Is the minimum size of the field.
 * 
 * - precision Is the maximum size of the field.
 * 
 * Supported flags :
 * 
 * - l  With integer number the argument will be of type long.
 * 
 * 
 * Supported type :
 * 
 * - s  Null terminated string of char.
 * - S  Null terminated string of char in upper case.
 * - i  Integer number.
 * - d  Integer number.
 * - u  Unsigned number.
 * - x  Unsigned number in hex.
 * - X  Unsigned number in hex upper case.
 * - b  Binary number
 * - o  Octal number
 * - p  Pointer will be emitted with the prefix ->
 * - P  Pointer in upper case letter.
 * - f  Floating point number.
 * - B	Boolean value printed as True / False.
 *
 * @param outchar - Pointer to the function to output one char.
 * @param arg   - Argument for the output function.
 * @param fmt   - Format options for the list of parameters.
 * @param args  -List parameters.
 *
 * @return The number of char emitted.
 */
unsigned xvformat(void (*outchar)(void *,char),void *arg,const char * fmt,va_list _args)
{
    unsigned count = 0;
    int state = 0;
    int cc;
    char c;
    /*
     * Maximum size of one int in binary format .. + 1 (in 64 bit machine)
     */
    char buffer[65];
    char prefix[2] = {0,0};
    int prefixlen = 0;
    int width = 0 ,prec = 0;
    unsigned  flags = 0;
    unsigned radix = 2;
    int length=0 ;
    char * out = buffer;
    unsigned long value = 0;
	int padding;
#if XCFG_FORMAT_FLOAT
    double dbl;
    unsigned long dPart;
    int i;
    double arr;
#endif

#if VA_COPY
	va_list args;
    
    va_copy(args,_args);
#else
#define	args	_args
#endif

    while (*fmt)
    {
        c = *fmt++;

        if (c < ' ' || c > 'z')
            cc = (int)CH_OTHER;
        else
            cc = formatStates[c - ' '] & 0x0F;
        
        state = formatStates[cc * 8 + state] >> 4;


        switch (state)
        {
            default:
            case    ST_NORMAL:
                (*outchar)(arg,c);
                count++;
                break;
                
            case    ST_PERCENT:
                prefixlen = width = prec = 0;
                flags = 0;
                length = 0;
                break;

            case    ST_WIDTH:
                if (c == '*')
                    width = (int)va_arg(args,int);
                else
                    width = width * 10 + (c - '0');
                break;
                
            case    ST_DOT:
                break;

            case    ST_PRECIS:
                flags |= FLAG_PREC;
                if (c == '*')
                    prec = (int)va_arg(args,int);
                else
                    prec = prec * 10 + (c - '0');
                break;

            case    ST_SIZE:
                switch (c)
                {
                    default:
                        break;
                    case    'l':
                        flags |= FLAG_LONG;
                        break;
                }
                break;

            case    ST_FLAG:
                switch (c)
                {
                    default:
                        break;
                    case    '-':
                        flags |= FLAG_LEFT;
                        break;
                    case    '0':
                        flags |= FLAG_ZERO;
                        break;
                }
                break;
                
            case    ST_TYPE:

                switch (c)
                {
                    default:
                        length = 0;
                        break;
                        
                        /*
                         * Pointer upper case
                         */
                    case    'P':
                        flags |= FLAG_UPPER;
                        /* no break */
                        /*lint -fallthrough */
                        
                        /*
                         * Pointer we assume sizeof(int) == sizeof(void *)
                         */
                    case    'p':
                        flags |= FLAG_INTEGER;
                        radix = 16;
                        prec = sizeof(void *) * 2;
                        prefix[0] = '-';
                        prefix[1] = '>';
                        prefixlen = 2;
                        break;
                        
                        /*
                         * Binary number
                         */
                    case    'b':
                        flags |= FLAG_INTEGER;
                        radix = 2;
                        break;

                        /*
                         * Octal number
                         */
                    case    'o':
                        flags |= FLAG_INTEGER;
                        radix = 8;
                        break;

                        /*
                         * Hex number upper case letter.
                         */
                    case    'X':
                    	/* no break */
                        flags |= FLAG_UPPER;
                        /* no break */

                        /* lint -fallthrough */

                        /*
                         * Hex number lower case
                         */
                    case    'x':
                        flags |= FLAG_INTEGER;
                        radix = 16;
                        break;

                        /*
                         * Integer number radix 10
                         */
                    case    'd':
                    case    'i':
                        flags |= FLAG_DECIMAL | FLAG_INTEGER;
                        radix = 10;
                        break;

                        /*
                         * Unsigned number
                         */
                    case    'u':
                        flags |= FLAG_INTEGER;
                        radix = 10;
                        break;

                        /*
                         * Upper case string
                         */
                    case    'S':
                        flags |= FLAG_UPPER;
                        /* no break */
                        /*lint -fallthrough */
                        
                        /*
                         * Normal string
                         */
                    case    's':
                        out = va_arg(args,char *);
                        if (out == 0)
                            out = (char *)ms_null;
                        length = (int)xstrlen(out);
                        break;

                        /*
                         * Upper case char
                         */
                    case    'C':
                        flags |= FLAG_UPPER;
                        /* no break */
                        /* lint -fallthrough */

                        /*
                         * Char
                         */
                    case    'c':
                        out = buffer;
                        buffer[0] = (char)va_arg(args,int);
                        length = 1;
                        break;
                        
#if XCFG_FORMAT_FLOAT
                        /**
                         * Floating point number
                         */
                   case 'f':
                        if ((flags & FLAG_PREC) == 0)
                            prec = 6;
                        dbl = va_arg(args,double);
                        arr = 0.51;
                        for (i = 0 ; i < prec ; i++)
                            arr /= 10.0;

                        if (dbl < 0)
                        {
                            dbl -= arr;
                            value = (long)dbl;
                            dbl -= (long)value;
                            dbl = - dbl;
                        }
                        else
                        {
                            dbl += arr;
                            value = (long)dbl;
                            dbl -= (long)value;
                        }
                        
                        
                        
                        for (i = 0 ;i < prec  ;i++)
                            dbl *= 10.0;
                        dPart = (unsigned long)dbl;
                        
                        out = buffer + sizeof(buffer) - 1;
                        if (prec)
                        {
                            while (prec --)
                            {
                                *out -- = ms_digits[dPart % 10];
                                dPart /= (unsigned)10;
                                length ++;
                            }

                            *out -- = '.';
                            length ++;
                        }
                        flags |= FLAG_INTEGER|FLAG_BUFFER|FLAG_DECIMAL|FLAG_VALUE;
                        radix = 10;
                        prec = 0;
                        break;
#endif
                        
                        /**
                         * Boolean value
                         */
                    case 'B':
                    	if (va_arg(args,int) != 0)
                    		out = ms_true;
                    	else
                    		out = ms_false;

                        length = (int)xstrlen(out);
                        break;

                        
                }

                /*
                 * Process integer number
                 */
                if (flags & FLAG_INTEGER)
                {
                    if (prec == 0)
                        prec = 1;

                    if ((flags & FLAG_VALUE) == 0)
                    {
                        if (flags & FLAG_LONG)
                        {
                            value = (long)va_arg(args,long);
                        }
						else
						{
							if (flags & FLAG_DECIMAL)
								value = (long)va_arg(args,int);
							else
								value = (long)va_arg(args,unsigned int);
						}
                    }


                    if (flags & FLAG_DECIMAL)
                    {
                        if (((long)value) < 0)
                        {
                            value = ~value + 1;
                            flags |= FLAG_MINUS;
                        }
                    }

                    if ((flags & FLAG_BUFFER) == 0)
                        out = buffer + sizeof(buffer) - 1;

                    while (prec -- > 0 || value)
                    {
                        *out -- = ms_digits[(unsigned long)value % radix];
                        value /= radix;
                        length ++;
                    };

                    out++;

                    if (flags & FLAG_MINUS)
                    {
                        if (flags & FLAG_ZERO)
                        {
                            prefixlen = 1;
                            prefix[0] = '-';
                        }
                        else
                        {
                            *--out = '-';
                            length++;
                        }
                    }

                }

                if (width && length > width)
                {
                    length = width;
                }
                
                padding = width - (length + prefixlen);

                
                
                count += outBuffer(outchar,arg,prefix,prefixlen,0);
                if (!(flags & FLAG_LEFT))
                    count += outChars(outchar,arg,flags & FLAG_ZERO ? '0' : ' ' , padding);
                count += outBuffer(outchar,arg,out,length,flags);
                if (flags & FLAG_LEFT)
                    count += outChars(outchar,arg,flags & FLAG_ZERO ? '0' : ' ' , padding);

        }
    }
    
    va_end(args);
    
    return count;
}
/*lint -restore */


/**
 * Test function
 */
#ifdef XCFG_FORMAT_TEST
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void myPutchar(void *arg,char c)
{
    char ** s = (char **)arg;
    *(*s)++ = c;
}

static void myPrintf(char *buf,const char *fmt,va_list args)
{
    xvformat(myPutchar,(void *)&buf,fmt,args);
    *buf = 0;
}

static void testFormat(const char * fmt,...)
{
    char buf1[1024];
    char buf2[1024];
    va_list args;
    va_list list;
    va_start(args,fmt);

    va_copy(list,args);
    myPrintf(buf1,fmt,list);
    va_end(list);

    va_copy(list,args);
    vsprintf(buf2,fmt,args);
    va_end(list);

    va_end(args);
    (void)args;

    printf("'%s'\n'%s'\n",buf1,buf2);
    if (strcmp(buf1,buf2))
    {
        printf("Error in '%s'\n",fmt);
        exit(1);
    }

    
}

int main()
{
    printf("XFORMATC test\n\n");
    testFormat("Hello world");
    testFormat("Hello %s","World");
    testFormat("integer %05d %d %d",-7,7,-7);
    testFormat("Unsigned %u %lu",123,123Lu);
    testFormat("Octal %o %lo",123,123456L);
    testFormat("Hex %x %X %lX",0x1234,0xf0ad,0xf2345678L);
    testFormat("String %4.4s","Large");
    testFormat("String %*.*s",4,4,"Hello");
#if XCFG_FORMAT_FLOAT
    
    testFormat("Floating %6.2f",22.0/7.0);
    testFormat("Floating %6.2f",-22.0/7.0);
    testFormat("Floating %6.1f %6.2f",3.999,-3.999);
    testFormat("Floating %6.1f %6.0f",3.999,-3.999);
#endif
    printf("Test completed succesfuylly\n"); 

    return 0;
}

#endif

#endif
