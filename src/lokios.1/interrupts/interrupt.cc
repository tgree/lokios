#include "interrupt.h"
#include "pic.h"
#include "ioapic.h"
#include "lapic.h"
#include "kernel/thread.h"
#include "kernel/cpu.h"
#include "kernel/console.h"
#include "kernel/pmtimer.h"
#include "k++/string_stream.h"

using kernel::console::printf;

static kernel::tls_tcb*
undefined_interrupt_entry(uint64_t selector, uint64_t error_code)
{
    kernel::tls_tcb* tcb = kernel::get_current_tcb();
    kernel::fixed_string_stream<80> ss;
    ss.printf("Unregistered vector %lu error_code 0x%016lX tcb 0x%016lX",
              selector,error_code,(uint64_t)tcb);
    kernel::panic(ss);
}

static bool int126_test_succeeded = false;
static kernel::tls_tcb*
int126_test_interrupt_entry(uint64_t selector, uint64_t error_code,
    uint64_t rsp)
{
    kernel::kassert(rsp % 16 == 0);
    int126_test_succeeded = true;
    return kernel::get_current_tcb();
}

static kernel::tls_tcb*
nmi_handler(uint64_t selector, uint64_t error_code)
{
    printf("NMI received\n");
    kernel::lapic_eoi();
    return kernel::get_current_tcb();
}

static kernel::tls_tcb*
page_fault_handler(uint64_t selector, uint64_t error_code)
{
    kernel::tls_tcb* tcb = kernel::get_current_tcb();
    printf("Page fault\n");
    printf("RIP 0x%016lX Addr 0x%016lX\n",tcb->rip,mfcr2());
    kernel::panic("Panic\n");
}

extern "C" void _interrupt_entry_noop();
extern "C" void _interrupt_entry_ticker();
extern "C" void _interrupt_entry_scheduler_wakeup();
extern "C" void _interrupt_entry_tlb_shootdown();
extern "C" void _interrupt_entry_0();
extern "C" void _interrupt_entry_1();
extern "C" void _interrupt_entry_2();
extern "C" void _interrupt_entry_3();
extern "C" void _interrupt_entry_4();
extern "C" void _interrupt_entry_5();
extern "C" void _interrupt_entry_6();
extern "C" void _interrupt_entry_7();
extern "C" void _interrupt_entry_8();
extern "C" void _interrupt_entry_9();
extern "C" void _interrupt_entry_10();
extern "C" void _interrupt_entry_11();
extern "C" void _interrupt_entry_12();
extern "C" void _interrupt_entry_13();
extern "C" void _interrupt_entry_14();
extern "C" void _interrupt_entry_15();
extern "C" void _interrupt_entry_16();
extern "C" void _interrupt_entry_17();
extern "C" void _interrupt_entry_18();
extern "C" void _interrupt_entry_19();
extern "C" void _interrupt_entry_20();
extern "C" void _interrupt_entry_21();
extern "C" void _interrupt_entry_22();
extern "C" void _interrupt_entry_23();
extern "C" void _interrupt_entry_24();
extern "C" void _interrupt_entry_25();
extern "C" void _interrupt_entry_26();
extern "C" void _interrupt_entry_27();
extern "C" void _interrupt_entry_28();
extern "C" void _interrupt_entry_29();
extern "C" void _interrupt_entry_30();
extern "C" void _interrupt_entry_31();
extern "C" void _interrupt_entry_32();
extern "C" void _interrupt_entry_33();
extern "C" void _interrupt_entry_34();
extern "C" void _interrupt_entry_35();
extern "C" void _interrupt_entry_36();
extern "C" void _interrupt_entry_37();
extern "C" void _interrupt_entry_38();
extern "C" void _interrupt_entry_39();
extern "C" void _interrupt_entry_40();
extern "C" void _interrupt_entry_41();
extern "C" void _interrupt_entry_42();
extern "C" void _interrupt_entry_43();
extern "C" void _interrupt_entry_44();
extern "C" void _interrupt_entry_45();
extern "C" void _interrupt_entry_46();
extern "C" void _interrupt_entry_47();
extern "C" void _interrupt_entry_48();
extern "C" void _interrupt_entry_49();
extern "C" void _interrupt_entry_50();
extern "C" void _interrupt_entry_51();
extern "C" void _interrupt_entry_52();
extern "C" void _interrupt_entry_53();
extern "C" void _interrupt_entry_54();
extern "C" void _interrupt_entry_55();
extern "C" void _interrupt_entry_56();
extern "C" void _interrupt_entry_57();
extern "C" void _interrupt_entry_58();
extern "C" void _interrupt_entry_59();
extern "C" void _interrupt_entry_60();
extern "C" void _interrupt_entry_61();
extern "C" void _interrupt_entry_62();
extern "C" void _interrupt_entry_63();
extern "C" void _interrupt_entry_64();
extern "C" void _interrupt_entry_65();
extern "C" void _interrupt_entry_66();
extern "C" void _interrupt_entry_67();
extern "C" void _interrupt_entry_68();
extern "C" void _interrupt_entry_69();
extern "C" void _interrupt_entry_70();
extern "C" void _interrupt_entry_71();
extern "C" void _interrupt_entry_72();
extern "C" void _interrupt_entry_73();
extern "C" void _interrupt_entry_74();
extern "C" void _interrupt_entry_75();
extern "C" void _interrupt_entry_76();
extern "C" void _interrupt_entry_77();
extern "C" void _interrupt_entry_78();
extern "C" void _interrupt_entry_79();
extern "C" void _interrupt_entry_80();
extern "C" void _interrupt_entry_81();
extern "C" void _interrupt_entry_82();
extern "C" void _interrupt_entry_83();
extern "C" void _interrupt_entry_84();
extern "C" void _interrupt_entry_85();
extern "C" void _interrupt_entry_86();
extern "C" void _interrupt_entry_87();
extern "C" void _interrupt_entry_88();
extern "C" void _interrupt_entry_89();
extern "C" void _interrupt_entry_90();
extern "C" void _interrupt_entry_91();
extern "C" void _interrupt_entry_92();
extern "C" void _interrupt_entry_93();
extern "C" void _interrupt_entry_94();
extern "C" void _interrupt_entry_95();
extern "C" void _interrupt_entry_96();
extern "C" void _interrupt_entry_97();
extern "C" void _interrupt_entry_98();
extern "C" void _interrupt_entry_99();
extern "C" void _interrupt_entry_100();
extern "C" void _interrupt_entry_101();
extern "C" void _interrupt_entry_102();
extern "C" void _interrupt_entry_103();
extern "C" void _interrupt_entry_104();
extern "C" void _interrupt_entry_105();
extern "C" void _interrupt_entry_106();
extern "C" void _interrupt_entry_107();
extern "C" void _interrupt_entry_108();
extern "C" void _interrupt_entry_109();
extern "C" void _interrupt_entry_110();
extern "C" void _interrupt_entry_111();
extern "C" void _interrupt_entry_112();
extern "C" void _interrupt_entry_113();
extern "C" void _interrupt_entry_114();
extern "C" void _interrupt_entry_115();
extern "C" void _interrupt_entry_116();
extern "C" void _interrupt_entry_117();
extern "C" void _interrupt_entry_118();
extern "C" void _interrupt_entry_119();
extern "C" void _interrupt_entry_120();
extern "C" void _interrupt_entry_121();
extern "C" void _interrupt_entry_122();
extern "C" void _interrupt_entry_123();
extern "C" void _interrupt_entry_124();
extern "C" void _interrupt_entry_125();
extern "C" void _interrupt_entry_126();
extern "C" void _interrupt_entry_127();
extern "C" void _msix_entry_128();
extern "C" void _msix_entry_129();
extern "C" void _msix_entry_130();
extern "C" void _msix_entry_131();
extern "C" void _msix_entry_132();
extern "C" void _msix_entry_133();
extern "C" void _msix_entry_134();
extern "C" void _msix_entry_135();
extern "C" void _msix_entry_136();
extern "C" void _msix_entry_137();
extern "C" void _msix_entry_138();
extern "C" void _msix_entry_139();
extern "C" void _msix_entry_140();
extern "C" void _msix_entry_141();
extern "C" void _msix_entry_142();
extern "C" void _msix_entry_143();
extern "C" void _msix_entry_144();
extern "C" void _msix_entry_145();
extern "C" void _msix_entry_146();
extern "C" void _msix_entry_147();
extern "C" void _msix_entry_148();
extern "C" void _msix_entry_149();
extern "C" void _msix_entry_150();
extern "C" void _msix_entry_151();
extern "C" void _msix_entry_152();
extern "C" void _msix_entry_153();
extern "C" void _msix_entry_154();
extern "C" void _msix_entry_155();
extern "C" void _msix_entry_156();
extern "C" void _msix_entry_157();
extern "C" void _msix_entry_158();
extern "C" void _msix_entry_159();
extern "C" void _msix_entry_160();
extern "C" void _msix_entry_161();
extern "C" void _msix_entry_162();
extern "C" void _msix_entry_163();
extern "C" void _msix_entry_164();
extern "C" void _msix_entry_165();
extern "C" void _msix_entry_166();
extern "C" void _msix_entry_167();
extern "C" void _msix_entry_168();
extern "C" void _msix_entry_169();
extern "C" void _msix_entry_170();
extern "C" void _msix_entry_171();
extern "C" void _msix_entry_172();
extern "C" void _msix_entry_173();
extern "C" void _msix_entry_174();
extern "C" void _msix_entry_175();
extern "C" void _msix_entry_176();
extern "C" void _msix_entry_177();
extern "C" void _msix_entry_178();
extern "C" void _msix_entry_179();
extern "C" void _msix_entry_180();
extern "C" void _msix_entry_181();
extern "C" void _msix_entry_182();
extern "C" void _msix_entry_183();
extern "C" void _msix_entry_184();
extern "C" void _msix_entry_185();
extern "C" void _msix_entry_186();
extern "C" void _msix_entry_187();
extern "C" void _msix_entry_188();
extern "C" void _msix_entry_189();
extern "C" void _msix_entry_190();
extern "C" void _msix_entry_191();
extern "C" void _msix_entry_192();
extern "C" void _msix_entry_193();
extern "C" void _msix_entry_194();
extern "C" void _msix_entry_195();
extern "C" void _msix_entry_196();
extern "C" void _msix_entry_197();
extern "C" void _msix_entry_198();
extern "C" void _msix_entry_199();
extern "C" void _msix_entry_200();
extern "C" void _msix_entry_201();
extern "C" void _msix_entry_202();
extern "C" void _msix_entry_203();
extern "C" void _msix_entry_204();
extern "C" void _msix_entry_205();
extern "C" void _msix_entry_206();
extern "C" void _msix_entry_207();
extern "C" void _msix_entry_208();
extern "C" void _msix_entry_209();
extern "C" void _msix_entry_210();
extern "C" void _msix_entry_211();
extern "C" void _msix_entry_212();
extern "C" void _msix_entry_213();
extern "C" void _msix_entry_214();
extern "C" void _msix_entry_215();
extern "C" void _msix_entry_216();
extern "C" void _msix_entry_217();
extern "C" void _msix_entry_218();
extern "C" void _msix_entry_219();
extern "C" void _msix_entry_220();
extern "C" void _msix_entry_221();
extern "C" void _msix_entry_222();
extern "C" void _msix_entry_223();
extern "C" void _msix_entry_224();
extern "C" void _msix_entry_225();
extern "C" void _msix_entry_226();
extern "C" void _msix_entry_227();
extern "C" void _msix_entry_228();
extern "C" void _msix_entry_229();
extern "C" void _msix_entry_230();
extern "C" void _msix_entry_231();
extern "C" void _msix_entry_232();
extern "C" void _msix_entry_233();
extern "C" void _msix_entry_234();
extern "C" void _msix_entry_235();
extern "C" void _msix_entry_236();
extern "C" void _msix_entry_237();
extern "C" void _msix_entry_238();
extern "C" void _msix_entry_239();
extern "C" void _msix_entry_240();
extern "C" void _msix_entry_241();
extern "C" void _msix_entry_242();
extern "C" void _msix_entry_243();
extern "C" void _msix_entry_244();
extern "C" void _msix_entry_245();
extern "C" void _msix_entry_246();
extern "C" void _msix_entry_247();
extern "C" void _msix_entry_248();
extern "C" void _msix_entry_249();
extern "C" void _msix_entry_250();
extern "C" void _msix_entry_251();
extern "C" void _msix_entry_252();
extern "C" void _msix_entry_253();
extern "C" void _msix_entry_254();
extern "C" void _msix_entry_255();

kernel::interrupt_handler _interrupt_handlers[128] = {
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
    undefined_interrupt_entry,
};

void
kernel::init_interrupts()
{
    register_handler(INTN_INT126_TEST,
                     (interrupt_handler)int126_test_interrupt_entry);
    register_handler(2,nmi_handler);
    register_handler(14,page_fault_handler);
}

void
kernel::init_cpu_interrupts()
{
    // Now: there could be pending interrupts if, say, a timer interrupt
    // previously fired.  Those interrupt vectors are already latched by the
    // CPU and they are going to trigger an interrupt vector no matter what at
    // this point when we enable the IF bit.  So, we start by setting up dummy
    // handlers for all the external interrupts, set the IF bit which will
    // cause the dummy handler to be called and drain the pending interrupt
    // queue and then we can finally set the real vectors we want in those
    // slots.
    cpu* c = get_current_cpu();
    for (size_t i=0; i<nelems(c->idt); ++i)
        c->register_exception_vector(i,_interrupt_entry_noop);

    // Enable interrupts.  This is going to trigger any pending external
    // interrupts which we will ignore.
    cpu_enable_interrupts();
    pmtimer::wait_us(100);

    // Now that we have drained the external interrupts and the interrupt
    // controllers are all masked.
    c->register_exception_vector(0,_interrupt_entry_0);
    c->register_exception_vector(1,_interrupt_entry_1);
    c->register_exception_vector(2,_interrupt_entry_2);
    c->register_exception_vector(3,_interrupt_entry_3);
    c->register_exception_vector(4,_interrupt_entry_4);
    c->register_exception_vector(5,_interrupt_entry_5);
    c->register_exception_vector(6,_interrupt_entry_6);
    c->register_exception_vector(7,_interrupt_entry_7);
    c->register_exception_vector(8,_interrupt_entry_8);
    c->register_exception_vector(9,_interrupt_entry_9);
    c->register_exception_vector(10,_interrupt_entry_10);
    c->register_exception_vector(11,_interrupt_entry_11);
    c->register_exception_vector(12,_interrupt_entry_12);
    c->register_exception_vector(13,_interrupt_entry_13);
    c->register_exception_vector(14,_interrupt_entry_14);
    c->register_exception_vector(15,_interrupt_entry_15);
    c->register_exception_vector(16,_interrupt_entry_16);
    c->register_exception_vector(17,_interrupt_entry_17);
    c->register_exception_vector(18,_interrupt_entry_18);
    c->register_exception_vector(19,_interrupt_entry_19);
    c->register_exception_vector(20,_interrupt_entry_20);
    c->register_exception_vector(21,_interrupt_entry_21);
    c->register_exception_vector(22,_interrupt_entry_22);
    c->register_exception_vector(23,_interrupt_entry_23);
    c->register_exception_vector(24,_interrupt_entry_24);
    c->register_exception_vector(25,_interrupt_entry_25);
    c->register_exception_vector(26,_interrupt_entry_26);
    c->register_exception_vector(27,_interrupt_entry_27);
    c->register_exception_vector(28,_interrupt_entry_28);
    c->register_exception_vector(29,_interrupt_entry_29);
    c->register_exception_vector(30,_interrupt_entry_30);
    c->register_exception_vector(31,_interrupt_entry_31);
    c->register_exception_vector(32,_interrupt_entry_32);
    c->register_exception_vector(33,_interrupt_entry_33);
    c->register_exception_vector(34,_interrupt_entry_34);
    c->register_exception_vector(35,_interrupt_entry_35);
    c->register_exception_vector(36,_interrupt_entry_36);
    c->register_exception_vector(37,_interrupt_entry_37);
    c->register_exception_vector(38,_interrupt_entry_38);
    c->register_exception_vector(39,_interrupt_entry_39);
    c->register_exception_vector(40,_interrupt_entry_40);
    c->register_exception_vector(41,_interrupt_entry_41);
    c->register_exception_vector(42,_interrupt_entry_42);
    c->register_exception_vector(43,_interrupt_entry_43);
    c->register_exception_vector(44,_interrupt_entry_44);
    c->register_exception_vector(45,_interrupt_entry_45);
    c->register_exception_vector(46,_interrupt_entry_46);
    c->register_exception_vector(47,_interrupt_entry_47);
    c->register_exception_vector(48,_interrupt_entry_48);
    c->register_exception_vector(49,_interrupt_entry_49);
    c->register_exception_vector(50,_interrupt_entry_50);
    c->register_exception_vector(51,_interrupt_entry_51);
    c->register_exception_vector(52,_interrupt_entry_52);
    c->register_exception_vector(53,_interrupt_entry_53);
    c->register_exception_vector(54,_interrupt_entry_54);
    c->register_exception_vector(55,_interrupt_entry_55);
    c->register_exception_vector(56,_interrupt_entry_56);
    c->register_exception_vector(57,_interrupt_entry_57);
    c->register_exception_vector(58,_interrupt_entry_58);
    c->register_exception_vector(59,_interrupt_entry_59);
    c->register_exception_vector(60,_interrupt_entry_60);
    c->register_exception_vector(61,_interrupt_entry_61);
    c->register_exception_vector(62,_interrupt_entry_62);
    c->register_exception_vector(63,_interrupt_entry_63);
    c->register_exception_vector(64,_interrupt_entry_64);
    c->register_exception_vector(65,_interrupt_entry_65);
    c->register_exception_vector(66,_interrupt_entry_66);
    c->register_exception_vector(67,_interrupt_entry_67);
    c->register_exception_vector(68,_interrupt_entry_68);
    c->register_exception_vector(69,_interrupt_entry_69);
    c->register_exception_vector(70,_interrupt_entry_70);
    c->register_exception_vector(71,_interrupt_entry_71);
    c->register_exception_vector(72,_interrupt_entry_72);
    c->register_exception_vector(73,_interrupt_entry_73);
    c->register_exception_vector(74,_interrupt_entry_74);
    c->register_exception_vector(75,_interrupt_entry_75);
    c->register_exception_vector(76,_interrupt_entry_76);
    c->register_exception_vector(77,_interrupt_entry_77);
    c->register_exception_vector(78,_interrupt_entry_78);
    c->register_exception_vector(79,_interrupt_entry_79);
    c->register_exception_vector(80,_interrupt_entry_80);
    c->register_exception_vector(81,_interrupt_entry_81);
    c->register_exception_vector(82,_interrupt_entry_82);
    c->register_exception_vector(83,_interrupt_entry_83);
    c->register_exception_vector(84,_interrupt_entry_84);
    c->register_exception_vector(85,_interrupt_entry_85);
    c->register_exception_vector(86,_interrupt_entry_86);
    c->register_exception_vector(87,_interrupt_entry_87);
    c->register_exception_vector(88,_interrupt_entry_88);
    c->register_exception_vector(89,_interrupt_entry_89);
    c->register_exception_vector(90,_interrupt_entry_90);
    c->register_exception_vector(91,_interrupt_entry_91);
    c->register_exception_vector(92,_interrupt_entry_92);
    c->register_exception_vector(93,_interrupt_entry_93);
    c->register_exception_vector(94,_interrupt_entry_94);
    c->register_exception_vector(95,_interrupt_entry_95);
    c->register_exception_vector(96,_interrupt_entry_96);
    c->register_exception_vector(97,_interrupt_entry_97);
    c->register_exception_vector(98,_interrupt_entry_98);
    c->register_exception_vector(99,_interrupt_entry_99);
    c->register_exception_vector(100,_interrupt_entry_100);
    c->register_exception_vector(101,_interrupt_entry_101);
    c->register_exception_vector(102,_interrupt_entry_102);
    c->register_exception_vector(103,_interrupt_entry_103);
    c->register_exception_vector(104,_interrupt_entry_104);
    c->register_exception_vector(105,_interrupt_entry_105);
    c->register_exception_vector(106,_interrupt_entry_106);
    c->register_exception_vector(107,_interrupt_entry_107);
    c->register_exception_vector(108,_interrupt_entry_108);
    c->register_exception_vector(109,_interrupt_entry_109);
    c->register_exception_vector(110,_interrupt_entry_110);
    c->register_exception_vector(111,_interrupt_entry_111);
    c->register_exception_vector(112,_interrupt_entry_112);
    c->register_exception_vector(113,_interrupt_entry_113);
    c->register_exception_vector(114,_interrupt_entry_114);
    c->register_exception_vector(115,_interrupt_entry_115);
    c->register_exception_vector(116,_interrupt_entry_116);
    c->register_exception_vector(117,_interrupt_entry_117);
    c->register_exception_vector(118,_interrupt_entry_118);
    c->register_exception_vector(119,_interrupt_entry_119);
    c->register_exception_vector(120,_interrupt_entry_120);
    c->register_exception_vector(121,_interrupt_entry_121);
    c->register_exception_vector(122,_interrupt_entry_tlb_shootdown);
    c->register_exception_vector(123,_interrupt_entry_scheduler_wakeup);
    c->register_exception_vector(124,_interrupt_entry_ticker);
    c->register_exception_vector(125,_interrupt_entry_125);
    c->register_exception_vector(126,_interrupt_entry_126);
    c->register_exception_vector(127,_interrupt_entry_127);
    c->register_exception_vector(128,_msix_entry_128);
    c->register_exception_vector(129,_msix_entry_129);
    c->register_exception_vector(130,_msix_entry_130);
    c->register_exception_vector(131,_msix_entry_131);
    c->register_exception_vector(132,_msix_entry_132);
    c->register_exception_vector(133,_msix_entry_133);
    c->register_exception_vector(134,_msix_entry_134);
    c->register_exception_vector(135,_msix_entry_135);
    c->register_exception_vector(136,_msix_entry_136);
    c->register_exception_vector(137,_msix_entry_137);
    c->register_exception_vector(138,_msix_entry_138);
    c->register_exception_vector(139,_msix_entry_139);
    c->register_exception_vector(140,_msix_entry_140);
    c->register_exception_vector(141,_msix_entry_141);
    c->register_exception_vector(142,_msix_entry_142);
    c->register_exception_vector(143,_msix_entry_143);
    c->register_exception_vector(144,_msix_entry_144);
    c->register_exception_vector(145,_msix_entry_145);
    c->register_exception_vector(146,_msix_entry_146);
    c->register_exception_vector(147,_msix_entry_147);
    c->register_exception_vector(148,_msix_entry_148);
    c->register_exception_vector(149,_msix_entry_149);
    c->register_exception_vector(150,_msix_entry_150);
    c->register_exception_vector(151,_msix_entry_151);
    c->register_exception_vector(152,_msix_entry_152);
    c->register_exception_vector(153,_msix_entry_153);
    c->register_exception_vector(154,_msix_entry_154);
    c->register_exception_vector(155,_msix_entry_155);
    c->register_exception_vector(156,_msix_entry_156);
    c->register_exception_vector(157,_msix_entry_157);
    c->register_exception_vector(158,_msix_entry_158);
    c->register_exception_vector(159,_msix_entry_159);
    c->register_exception_vector(160,_msix_entry_160);
    c->register_exception_vector(161,_msix_entry_161);
    c->register_exception_vector(162,_msix_entry_162);
    c->register_exception_vector(163,_msix_entry_163);
    c->register_exception_vector(164,_msix_entry_164);
    c->register_exception_vector(165,_msix_entry_165);
    c->register_exception_vector(166,_msix_entry_166);
    c->register_exception_vector(167,_msix_entry_167);
    c->register_exception_vector(168,_msix_entry_168);
    c->register_exception_vector(169,_msix_entry_169);
    c->register_exception_vector(170,_msix_entry_170);
    c->register_exception_vector(171,_msix_entry_171);
    c->register_exception_vector(172,_msix_entry_172);
    c->register_exception_vector(173,_msix_entry_173);
    c->register_exception_vector(174,_msix_entry_174);
    c->register_exception_vector(175,_msix_entry_175);
    c->register_exception_vector(176,_msix_entry_176);
    c->register_exception_vector(177,_msix_entry_177);
    c->register_exception_vector(178,_msix_entry_178);
    c->register_exception_vector(179,_msix_entry_179);
    c->register_exception_vector(180,_msix_entry_180);
    c->register_exception_vector(181,_msix_entry_181);
    c->register_exception_vector(182,_msix_entry_182);
    c->register_exception_vector(183,_msix_entry_183);
    c->register_exception_vector(184,_msix_entry_184);
    c->register_exception_vector(185,_msix_entry_185);
    c->register_exception_vector(186,_msix_entry_186);
    c->register_exception_vector(187,_msix_entry_187);
    c->register_exception_vector(188,_msix_entry_188);
    c->register_exception_vector(189,_msix_entry_189);
    c->register_exception_vector(190,_msix_entry_190);
    c->register_exception_vector(191,_msix_entry_191);
    c->register_exception_vector(192,_msix_entry_192);
    c->register_exception_vector(193,_msix_entry_193);
    c->register_exception_vector(194,_msix_entry_194);
    c->register_exception_vector(195,_msix_entry_195);
    c->register_exception_vector(196,_msix_entry_196);
    c->register_exception_vector(197,_msix_entry_197);
    c->register_exception_vector(198,_msix_entry_198);
    c->register_exception_vector(199,_msix_entry_199);
    c->register_exception_vector(200,_msix_entry_200);
    c->register_exception_vector(201,_msix_entry_201);
    c->register_exception_vector(202,_msix_entry_202);
    c->register_exception_vector(203,_msix_entry_203);
    c->register_exception_vector(204,_msix_entry_204);
    c->register_exception_vector(205,_msix_entry_205);
    c->register_exception_vector(206,_msix_entry_206);
    c->register_exception_vector(207,_msix_entry_207);
    c->register_exception_vector(208,_msix_entry_208);
    c->register_exception_vector(209,_msix_entry_209);
    c->register_exception_vector(210,_msix_entry_210);
    c->register_exception_vector(211,_msix_entry_211);
    c->register_exception_vector(212,_msix_entry_212);
    c->register_exception_vector(213,_msix_entry_213);
    c->register_exception_vector(214,_msix_entry_214);
    c->register_exception_vector(215,_msix_entry_215);
    c->register_exception_vector(216,_msix_entry_216);
    c->register_exception_vector(217,_msix_entry_217);
    c->register_exception_vector(218,_msix_entry_218);
    c->register_exception_vector(219,_msix_entry_219);
    c->register_exception_vector(220,_msix_entry_220);
    c->register_exception_vector(221,_msix_entry_221);
    c->register_exception_vector(222,_msix_entry_222);
    c->register_exception_vector(223,_msix_entry_223);
    c->register_exception_vector(224,_msix_entry_224);
    c->register_exception_vector(225,_msix_entry_225);
    c->register_exception_vector(226,_msix_entry_226);
    c->register_exception_vector(227,_msix_entry_227);
    c->register_exception_vector(228,_msix_entry_228);
    c->register_exception_vector(229,_msix_entry_229);
    c->register_exception_vector(230,_msix_entry_230);
    c->register_exception_vector(231,_msix_entry_231);
    c->register_exception_vector(232,_msix_entry_232);
    c->register_exception_vector(233,_msix_entry_233);
    c->register_exception_vector(234,_msix_entry_234);
    c->register_exception_vector(235,_msix_entry_235);
    c->register_exception_vector(236,_msix_entry_236);
    c->register_exception_vector(237,_msix_entry_237);
    c->register_exception_vector(238,_msix_entry_238);
    c->register_exception_vector(239,_msix_entry_239);
    c->register_exception_vector(240,_msix_entry_240);
    c->register_exception_vector(241,_msix_entry_241);
    c->register_exception_vector(242,_msix_entry_242);
    c->register_exception_vector(243,_msix_entry_243);
    c->register_exception_vector(244,_msix_entry_244);
    c->register_exception_vector(245,_msix_entry_245);
    c->register_exception_vector(246,_msix_entry_246);
    c->register_exception_vector(247,_msix_entry_247);
    c->register_exception_vector(248,_msix_entry_248);
    c->register_exception_vector(249,_msix_entry_249);
    c->register_exception_vector(250,_msix_entry_250);
    c->register_exception_vector(251,_msix_entry_251);
    c->register_exception_vector(252,_msix_entry_252);
    c->register_exception_vector(253,_msix_entry_253);
    c->register_exception_vector(254,_msix_entry_254);
    c->register_exception_vector(255,_msix_entry_255);

    // Generate a software interrupt to check that we have basic exception-
    // handling working properly.
    int126();
    kassert(int126_test_succeeded == true);
    printf("INT 126h test succeeded\n");
}

void
kernel::register_handler(uint64_t selector, interrupt_handler handler)
{
    kassert(selector < nelems(_interrupt_handlers));
    kassert(_interrupt_handlers[selector] == undefined_interrupt_entry);
    _interrupt_handlers[selector] = handler;
}

void
kernel::unregister_handler(uint64_t selector)
{
    kassert(selector < nelems(_interrupt_handlers));
    kassert(_interrupt_handlers[selector] != undefined_interrupt_entry);
    _interrupt_handlers[selector] = undefined_interrupt_entry;
}
