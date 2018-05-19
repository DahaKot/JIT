#include <iostream>
#include <sys/mman.h>
#include <zconf.h>
#include <assert.h>

static const size_t max_plen = 1000;

void Set_Const(char *target, size_t *j, size_t adr);

enum Registers {
    rax = 41,
    rbx = 42,
    rcx = 43,
    rdx = 44
};

enum COMMAND_CODES {
    PUSH_NUMBER = 1,
    POP_REG = 2,
    ADD = 3,
    SUB = 4,
    MUL = 5,
    DIV = 6,
    MOD = 7,
    IDIV = 8,
    OUT = 9,
    IN = 10,
    PUSH_REG = 11,
    SQRT = 12,
    END = 13,
    SIN = 14,
    COS = 15,
    LOG = 16,
    POW = 17,
    LABEL = 18,
    JMP = 19,
    JE = 20,
    JNE = 21,
    JA = 22,
    JAE = 23,
    JB = 24,
    JBE = 25,
    CALL = 26,
    RET = 27,
    MEOW = 28,
    PUSH_RAM_REG = 29,              //push [ax]
    PUSH_RAM = 30,                  //push [0]
    POP_RAM = 31,                   //pop [0]
    POP_RAM_REG = 32,               //pop [ax]
    PIZZA = 33,
    OUTA = 34,
    ENTER = 35,
    LEAVE = 36
};


enum p_cmd {
    ret = 0xc3,
    push_rax = 0x50,
    push_rbx = 0x53,
    push_rcx = 0x51,
    push_rdx = 0x52,
    push_snum= 0x6a,
    push_bnum= 0x68,
    abs_jmp  = 0xff,
    je       = 0x74,
    jne      = 0x75,
    ja       = 0x77,
    jae      = 0x73,
    jb       = 0x72,
    jbe      = 0x76,
    rel_jmp  = 0xeb,
    call     = 0xe8,
    rel_long_jmp = 0xe9
};

const long _add = 4938558558960097316;
const long _sub = 4938558559631185956;
const long _mul = 4938558610331932708;

#define pop(x) (push_##x + 0x8)
#define push_reg    if (program[i] == rax) {           \
                        target[j++] = push_rax; \
                    }                           \
                    else if (program[i] == rbx) {      \
                        target[j++] = push_rbx; \
                    }                           \
                    else if (program[i] == rcx) {      \
                        target[j++] = push_rcx; \
                    }                           \
                    else if (program[i] == rdx) {      \
                        target[j++] = push_rdx; \
                    }

#define pop_reg     if (program[i] == rax) {           \
                        target[j++] = pop(rax); \
                    }                           \
                    else if (program[i] == rbx) {      \
                        target[j++] = pop(rbx); \
                    }                           \
                    else if (program[i] == rcx) {      \
                        target[j++] = pop(rcx); \
                    }                           \
                    else if (program[i] == rdx) {      \
                        target[j++] = pop(rdx); \
                    }

#define absolute_jmp    i++;                                    \
                        jmp_adrs[k] = program[i];               \
                        k++;                                    \
                        target[j++] = rel_long_jmp;             \
                        target[j++] = 0;                        \
                        target[j++] = 0;                        \
                        target[j++] = 0;                        \
                        target[j++] = 0;                        \
                        break;

#define conditional_jmp(c)  i++;                                    \
                            jmp_adrs[k] = program[i];               \
                            k++;                                    \
                            (*(long *) (target + j)) = 0x2474394c24348b4c;\
                            j+=8;\
                            (*(long *) (target + j)) = 0x5e415e4108;\
                            j+=5;\
                            target[j++] = j##c;                     \
                            target[j++] = 0x2;                      \
                            target[j++] = rel_jmp;                  \
                            target[j++] = 0x5;                      \
                            target[j++] = rel_long_jmp;             \
                            target[j++] = 0;                        \
                            target[j++] = 0;                        \
                            target[j++] = 0;                        \
                            target[j++] = 0;                        \
                            break;

#define set_const           target[i++] = adr & 0xff;               \
                            adr = adr >> 8;                         \
                            target[i++] = adr & 0xff;               \
                            adr = adr >> 8;                         \
                            target[i++] = adr & 0xff;               \
                            adr = adr >> 8;                         \
                            target[i] = adr & 0xff;

#define cmd_do(x)           target[j++] = push_rax;                         \
                            target[j++] = push_rbx;                         \
                            (*(long *) (target + j)) = 4939120650508602184; \
                            j += 8;                                         \
                            (*(long *) (target + j)) = _##x;                \
                            j += 8;                                         \
                            (*(long *) (target + j)) = 631774194481960996;  \
                            j += 8; \
                            break;

size_t *Make_array(char *source);

int main() {
    size_t page_size = sysconf(_SC_PAGE_SIZE);

    char source[] = "1 3 "
            "26 5 "
            "27 "
            "35 "
            "2 41 "
            "11 41 "
            "11 41 "
            "11 41 "
            "1 1 "
            "22 22 "
            "2 41 "
            "36 "
            "27 "
            "1 1 "
            "4 "
            "26 5 "
            "5 "
            "19 20 "
            "27";
    char *target = nullptr;
    std::cout << posix_memalign((void**) &target, page_size, 3*page_size);     //align to page adr
    std::cout << "\n";

    size_t *program = Make_array(source);

    int *cmds = new int[max_plen];
    for (int i = 0; i < max_plen; i++) {
        cmds[i] = -1;
    }
    cmds[0] = 0;

    size_t *jmp_adrs = new size_t[max_plen];

    size_t j = 0;
    size_t buff_for_printf = (size_t) (target + 1024);
    (*(long *) (target + j)) = 0x80ae9;
    j += 5;
    printf("%x\n", target[2]);
    for (size_t t = 0; t < 256; t++) {
        (*(long *) (target + j)) = 0;
        j += 8;
    }
    (*(long *) (target + j)) = 0x00695c6425;
    size_t printf_str_adr = (size_t) target + j;
    j += 5;
    (*(long *) (target + j)) = 0x9090909090006425;
    j += 8;
    (*(long *) (target + j)) = 0xbf41;
    j += 2;
    size_t call_stack = (size_t) (target + j + 1028);
    Set_Const(target, &j, call_stack);
    //printf 5548 89e5 48
    /*(*(long *) (target + j)) = 0x48e5894855;       j += 5;
    (*(long *) (target + j)) = 0xbf10758b;         j += 4;
    target[j++] = buff_for_printf & 0xff;
    printf("target %d\n", target[j-1]);
    buff_for_printf = buff_for_printf >> 8;
    target[j++] = buff_for_printf & 0xff;
    buff_for_printf = buff_for_printf >> 8;
    target[j++] = buff_for_printf & 0xff;
    buff_for_printf = buff_for_printf >> 8;
    target[j++] = buff_for_printf & 0xff;
    (*(long *) (target + j)) = 0x80c3ff48db3148;   j += 7;
    (*(long *) (target + j)) = 0x74253e803974003e; j += 8;
    (*(long *) (target + j)) = 0xc6ff48f2eba4fc04; j += 8;
    (*(long *) (target + j)) = 0x74643cc6ff48068a; j += 8;
    (*(long *) (target + j)) = 0xe813c7834822eb02; j += 8;
    (*(long *) (target + j)) = 0xb9c889480000007f; j += 8;
    (*(long *) (target + j)) = 0xb8c1294800000013; j += 8;
    (*(long *) (target + j)) = 0x48aaf3fd00000024; j += 8;
    (*(long *) (target + j)) = 0x0a07c6c2eb14c783; j += 8;
    (*(long *) (target + j)) = 0x0d8bb9//0000001ce8; j += 8;
    (*(long *) (target + j)) = 0x00c18148fffffe6b; j += 8;
    (*(long *) (target + j)) = 0x00000001bb//000000; j += 8;
    (*(long *) (target + j)) = 0x5d80cd00000004b8; j += 8;
    (*(long *) (target + j)) = 0xfffffd23358b48c3; j += 8;
    (*(long *) (target + j)) = 0x4800000000c68148; j += 8;
    (*(long *) (target + j)) = 0x8148fffffe413d8b; j += 8;
    (*(long *) (target + j)) = 0xd2314800000000c7; j += 8;
    (*(long *) (target + j)) = 0x243e8011740a3e80; j += 8;
    (*(long *) (target + j)) = 0xebc2ff48a4fc0774; j += 8;
    (*(long *) (target + j)) = 0x06c6eaebc6ff48ef; j += 8;
    (*(long *) (target + j)) = 0xa40374003e80c300; j += 8;
    (*(long *) (target + j)) = 0x48db314d53c3f8eb; j += 8;
    (*(long *) (target + j)) = 0x48c3894810dd448b; j += 8;
    (*(long *) (target + j)) = 0x480000000ab9d231; j += 8;
    (*(long *) (target + j)) = 0x48178830c280f1f7; j += 8;
    (*(long *) (target + j)) = 0xd23148c3ff49cfff; j += 8;
    (*(long *) (target + j)) = 0xe2eb027400f88348; j += 8;
    (*(long *) (target + j)) = 0xc3d9894cc3ff485b; j += 8;
    */
    size_t ram_adr = 0;

    for (size_t i = 0, k = 0; program[i] != END; i++) {
        cmds[i] = (int) j;
        switch(program[i]) {
            case(RET):  target[j++] = ret;
                        break;
            case(PUSH_REG):
                        i++;
                        push_reg;//j++
                        break;
            case(PUSH_NUMBER):
                        i++;
                        if (program[i] < 256) {
                            target[j++] = push_snum;
                            target[j++] = program[i];
                        }
                        else {
                            assert(0);
                        }
                        break;
            case(POP_REG):
                        i++;
                        pop_reg;//j++
                        break;
            case(JMP):  absolute_jmp;//j++
            case(JE):   conditional_jmp(e);
                        /*target[j++] = je;                     //je--------
                        target[j++] = 0x2;                      //          |
                        target[j++] = rel_jmp;                  //jmp-----------
                        target[j++] = 0x7;                      //          |   |
                        target[j++] = abs_jmp;                  //jmp x   <--   |
                        target[j++] = 0x24;                     //              |
                        target[j++] = 0x25;                     //              |
                        target[j++] = cmds[cmd] % 256;          //              |
                        target[j++] = cmds[cmd] / 256;          //              |
                        target[j++] = 0;                        //              |
                        target[j++] = 0;                        //        <------*/
            case(JNE):  conditional_jmp(ne);
            case(JA):   conditional_jmp(a);
            case(JAE):  conditional_jmp(ae);
            case(JB):   conditional_jmp(b);
            case(JBE):  conditional_jmp(be);
            case(CALL): i++;
                        target[j++] = call;
                        jmp_adrs[k] = program[i];
                        k++;
                        j+=4;
                        break;
            case(ADD):  cmd_do(add);
            case(SUB):  cmd_do(sub);
            case(MUL):  cmd_do(mul);
            case(DIV):  target[j++] = push_rdx;
                target[j++] = 0x48;
                target[j++] = 0x31;
                target[j++] = 0xd2;
                        target[j++] = push_rax;
                        target[j++] = push_rbx;
                        (*(long *) (target + j)) = 4939120684868340552;
                        j += 8;
                        (*(long *) (target + j)) = 2613364630205440036;
                        j += 8;
                        (*(long *) (target + j)) = 631774194515335968;
                        j += 8;
                        break;
            case(PUSH_RAM): i++;
                        (*(long *) (target + j)) = 0x2534ff;
                        j += 3;
                        ram_adr = program[i]*4 + (size_t) target + ((program[i]%2 == 0) ? 8 : 12);
                            target[j++] = ram_adr & 0xff;
                            ram_adr = ram_adr >> 8;
                            target[j++] = ram_adr & 0xff;
                            ram_adr = ram_adr >> 8;
                            target[j++] = ram_adr & 0xff;
                            ram_adr = ram_adr >> 8;
                            target[j++] = ram_adr & 0xff;
                        break;
            case(POP_RAM): i++;
                        (*(long *) (target + j)) = 0x25048f;
                        j += 3;
                        ram_adr = program[i]*4 + (size_t) target + ((program[i]%2 == 0) ? 8 : 12);
                        target[j++] = ram_adr & 0xff;
                        ram_adr = ram_adr >> 8;
                        target[j++] = ram_adr & 0xff;
                        ram_adr = ram_adr >> 8;
                        target[j++] = ram_adr & 0xff;
                        ram_adr = ram_adr >> 8;
                        target[j++] = ram_adr & 0xff;
                        break;
            case(ENTER):(*(long *) (target + j)) = 0x078f4108c78349;
                        j += 7;
                        break;
            case(LEAVE):(*(long *) (target + j)) = 0x08ef834937ff41;
                        j += 7;
                        break;
            case(OUT):  target[j++] = push_bnum;
                        target[j++] = printf_str_adr & 0xff;
                        printf_str_adr = printf_str_adr >> 8;
                        target[j++] = printf_str_adr & 0xff;
                        printf_str_adr = printf_str_adr >> 8;
                        target[j++] = printf_str_adr & 0xff;
                        printf_str_adr = printf_str_adr >> 8;
                        target[j++] = printf_str_adr & 0xff;
            default:    break;
        }
    }

    size_t adr = 0;

    for (size_t i = 0, k = 0; i < j; i++) {
        fflush( stdout );
        if (target[i] == -23 && i != 0) {
            printf("%d\n", (char) rel_long_jmp);
            i++;
            adr = cmds[jmp_adrs[k]] - i - 4;//(size_t) target + cmds[jmp_adrs[k]];
            k++;
            set_const;
        }
        else if (target[i] == -24) {
            i++;
            adr = cmds[jmp_adrs[k]] - i - 4;
            k++;
            set_const;
        }
    }

    //for (size_t i = 0; i < 50; i++) {
    //    printf("%x\n", target[i]);
    //}


    void (*func) () = nullptr;                                  //try to exec target
    std::cout << mprotect(target, page_size, PROT_EXEC | PROT_WRITE);
    func = (void (*) ()) target;

    func();

    free(target);

    return 0;
}

size_t *Make_array(char *source) {
    size_t *arr = (size_t *) calloc(max_plen, sizeof(size_t));
    assert(arr);

    char *end_ptr = nullptr;
    size_t k = 0;
    for (size_t i = 0; source[i] != 0; i += end_ptr - source - i, k++) {
        arr[k] = strtol(source + i, &end_ptr, 10);
    }

    arr[k] = END;

    return arr;
}

void Set_Const(char *target, size_t *j, size_t adr) {
    target[(*j)++] = adr & 0xff;
    adr = adr >> 8;
    target[(*j)++] = adr & 0xff;
    adr = adr >> 8;
    target[(*j)++] = adr & 0xff;
    adr = adr >> 8;
    target[(*j)++] = adr & 0xff;
}