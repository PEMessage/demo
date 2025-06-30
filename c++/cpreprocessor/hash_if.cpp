


// comment two line below to test
#define A 3
#define B


// you could also use 'or', treat defined as function
#if defined(A) || defined(B) 
    #warning "Def A or B"
#else
    #error "A and B not defined"
#endif

#if defined(A) && (A == 3)
    #warning "A is 3"
#else
    #warning "A is not 3"
#endif

// you could reverse defined by !

// #define A_rev
// #define B_rev
#if !defined(A_rev) && !defined(B_rev) 
    #warning "Def A and B was not defined"
#else
    #error "A_rev or B_rev was defined"
#endif






// dummy main
int main (int argc, char *argv[]) {
    return 0;
}
