# 1 "test/test_1-4-b.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "test/test_1-4-b.c"
# 1 "/usr/lib/gcc/x86_64-linux-gnu/8/include/stdbool.h" 1 3 4
# 2 "test/test_1-4-b.c" 2

int main(int argc, char *argv)
{
    
# 5 "test/test_1-4-b.c" 3 4
   _Bool 
# 5 "test/test_1-4-b.c"
        a = (1 + 2) || (3 - 6);

    
# 7 "test/test_1-4-b.c" 3 4
   _Bool 
# 7 "test/test_1-4-b.c"
        b = 1 + 2 || (3 - 6);

    
# 9 "test/test_1-4-b.c" 3 4
   _Bool 
# 9 "test/test_1-4-b.c"
        c = (1 + 2) || 3 - 6;

    
# 11 "test/test_1-4-b.c" 3 4
   _Bool 
# 11 "test/test_1-4-b.c"
        d = (1 + 2) && (3 - 6) || (7 + 9);

    
# 13 "test/test_1-4-b.c" 3 4
   _Bool 
# 13 "test/test_1-4-b.c"
        a = (1 + 2) && (3 - 6);

    
# 15 "test/test_1-4-b.c" 3 4
   _Bool 
# 15 "test/test_1-4-b.c"
        b = 1 + 2 && (3 - 6);

    
# 17 "test/test_1-4-b.c" 3 4
   _Bool 
# 17 "test/test_1-4-b.c"
        c = (1 + 2) && 3 - 6;
}
