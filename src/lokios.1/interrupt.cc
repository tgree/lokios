#include "interrupt.h"
#include "thread.h"
#include "cpu.h"
#include "k++/string_stream.h"

static kernel::tls_tcb*
undefined_interrupt_entry(uint64_t selector, uint64_t error_code)
{
    kernel::tls_tcb* tcb = kernel::get_current_tcb();
    kernel::fixed_string_stream<80> ss;
    ss.printf("Unregistered vector %lu error_code 0x%016lX tcb 0x%016lX",
              selector,error_code,(uint64_t)tcb);
    kernel::panic(ss);
}

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
    for (auto cpu : cpus)
    {
        cpu->register_exception_vector(0,_interrupt_entry_0);
        cpu->register_exception_vector(1,_interrupt_entry_1);
        cpu->register_exception_vector(2,_interrupt_entry_2);
        cpu->register_exception_vector(3,_interrupt_entry_3);
        cpu->register_exception_vector(4,_interrupt_entry_4);
        cpu->register_exception_vector(5,_interrupt_entry_5);
        cpu->register_exception_vector(6,_interrupt_entry_6);
        cpu->register_exception_vector(7,_interrupt_entry_7);
        cpu->register_exception_vector(8,_interrupt_entry_8);
        cpu->register_exception_vector(9,_interrupt_entry_9);
        cpu->register_exception_vector(10,_interrupt_entry_10);
        cpu->register_exception_vector(11,_interrupt_entry_11);
        cpu->register_exception_vector(12,_interrupt_entry_12);
        cpu->register_exception_vector(13,_interrupt_entry_13);
        cpu->register_exception_vector(14,_interrupt_entry_14);
        cpu->register_exception_vector(15,_interrupt_entry_15);
        cpu->register_exception_vector(16,_interrupt_entry_16);
        cpu->register_exception_vector(17,_interrupt_entry_17);
        cpu->register_exception_vector(18,_interrupt_entry_18);
        cpu->register_exception_vector(19,_interrupt_entry_19);
        cpu->register_exception_vector(20,_interrupt_entry_20);
        cpu->register_exception_vector(21,_interrupt_entry_21);
        cpu->register_exception_vector(22,_interrupt_entry_22);
        cpu->register_exception_vector(23,_interrupt_entry_23);
        cpu->register_exception_vector(24,_interrupt_entry_24);
        cpu->register_exception_vector(25,_interrupt_entry_25);
        cpu->register_exception_vector(26,_interrupt_entry_26);
        cpu->register_exception_vector(27,_interrupt_entry_27);
        cpu->register_exception_vector(28,_interrupt_entry_28);
        cpu->register_exception_vector(29,_interrupt_entry_29);
        cpu->register_exception_vector(30,_interrupt_entry_30);
        cpu->register_exception_vector(31,_interrupt_entry_31);
        cpu->register_exception_vector(32,_interrupt_entry_32);
        cpu->register_exception_vector(33,_interrupt_entry_33);
        cpu->register_exception_vector(34,_interrupt_entry_34);
        cpu->register_exception_vector(35,_interrupt_entry_35);
        cpu->register_exception_vector(36,_interrupt_entry_36);
        cpu->register_exception_vector(37,_interrupt_entry_37);
        cpu->register_exception_vector(38,_interrupt_entry_38);
        cpu->register_exception_vector(39,_interrupt_entry_39);
        cpu->register_exception_vector(40,_interrupt_entry_40);
        cpu->register_exception_vector(41,_interrupt_entry_41);
        cpu->register_exception_vector(42,_interrupt_entry_42);
        cpu->register_exception_vector(43,_interrupt_entry_43);
        cpu->register_exception_vector(44,_interrupt_entry_44);
        cpu->register_exception_vector(45,_interrupt_entry_45);
        cpu->register_exception_vector(46,_interrupt_entry_46);
        cpu->register_exception_vector(47,_interrupt_entry_47);
        cpu->register_exception_vector(48,_interrupt_entry_48);
        cpu->register_exception_vector(49,_interrupt_entry_49);
        cpu->register_exception_vector(50,_interrupt_entry_50);
        cpu->register_exception_vector(51,_interrupt_entry_51);
        cpu->register_exception_vector(52,_interrupt_entry_52);
        cpu->register_exception_vector(53,_interrupt_entry_53);
        cpu->register_exception_vector(54,_interrupt_entry_54);
        cpu->register_exception_vector(55,_interrupt_entry_55);
        cpu->register_exception_vector(56,_interrupt_entry_56);
        cpu->register_exception_vector(57,_interrupt_entry_57);
        cpu->register_exception_vector(58,_interrupt_entry_58);
        cpu->register_exception_vector(59,_interrupt_entry_59);
        cpu->register_exception_vector(60,_interrupt_entry_60);
        cpu->register_exception_vector(61,_interrupt_entry_61);
        cpu->register_exception_vector(62,_interrupt_entry_62);
        cpu->register_exception_vector(63,_interrupt_entry_63);
        cpu->register_exception_vector(64,_interrupt_entry_64);
        cpu->register_exception_vector(65,_interrupt_entry_65);
        cpu->register_exception_vector(66,_interrupt_entry_66);
        cpu->register_exception_vector(67,_interrupt_entry_67);
        cpu->register_exception_vector(68,_interrupt_entry_68);
        cpu->register_exception_vector(69,_interrupt_entry_69);
        cpu->register_exception_vector(70,_interrupt_entry_70);
        cpu->register_exception_vector(71,_interrupt_entry_71);
        cpu->register_exception_vector(72,_interrupt_entry_72);
        cpu->register_exception_vector(73,_interrupt_entry_73);
        cpu->register_exception_vector(74,_interrupt_entry_74);
        cpu->register_exception_vector(75,_interrupt_entry_75);
        cpu->register_exception_vector(76,_interrupt_entry_76);
        cpu->register_exception_vector(77,_interrupt_entry_77);
        cpu->register_exception_vector(78,_interrupt_entry_78);
        cpu->register_exception_vector(79,_interrupt_entry_79);
        cpu->register_exception_vector(80,_interrupt_entry_80);
        cpu->register_exception_vector(81,_interrupt_entry_81);
        cpu->register_exception_vector(82,_interrupt_entry_82);
        cpu->register_exception_vector(83,_interrupt_entry_83);
        cpu->register_exception_vector(84,_interrupt_entry_84);
        cpu->register_exception_vector(85,_interrupt_entry_85);
        cpu->register_exception_vector(86,_interrupt_entry_86);
        cpu->register_exception_vector(87,_interrupt_entry_87);
        cpu->register_exception_vector(88,_interrupt_entry_88);
        cpu->register_exception_vector(89,_interrupt_entry_89);
        cpu->register_exception_vector(90,_interrupt_entry_90);
        cpu->register_exception_vector(91,_interrupt_entry_91);
        cpu->register_exception_vector(92,_interrupt_entry_92);
        cpu->register_exception_vector(93,_interrupt_entry_93);
        cpu->register_exception_vector(94,_interrupt_entry_94);
        cpu->register_exception_vector(95,_interrupt_entry_95);
        cpu->register_exception_vector(96,_interrupt_entry_96);
        cpu->register_exception_vector(97,_interrupt_entry_97);
        cpu->register_exception_vector(98,_interrupt_entry_98);
        cpu->register_exception_vector(99,_interrupt_entry_99);
        cpu->register_exception_vector(100,_interrupt_entry_100);
        cpu->register_exception_vector(101,_interrupt_entry_101);
        cpu->register_exception_vector(102,_interrupt_entry_102);
        cpu->register_exception_vector(103,_interrupt_entry_103);
        cpu->register_exception_vector(104,_interrupt_entry_104);
        cpu->register_exception_vector(105,_interrupt_entry_105);
        cpu->register_exception_vector(106,_interrupt_entry_106);
        cpu->register_exception_vector(107,_interrupt_entry_107);
        cpu->register_exception_vector(108,_interrupt_entry_108);
        cpu->register_exception_vector(109,_interrupt_entry_109);
        cpu->register_exception_vector(110,_interrupt_entry_110);
        cpu->register_exception_vector(111,_interrupt_entry_111);
        cpu->register_exception_vector(112,_interrupt_entry_112);
        cpu->register_exception_vector(113,_interrupt_entry_113);
        cpu->register_exception_vector(114,_interrupt_entry_114);
        cpu->register_exception_vector(115,_interrupt_entry_115);
        cpu->register_exception_vector(116,_interrupt_entry_116);
        cpu->register_exception_vector(117,_interrupt_entry_117);
        cpu->register_exception_vector(118,_interrupt_entry_118);
        cpu->register_exception_vector(119,_interrupt_entry_119);
        cpu->register_exception_vector(120,_interrupt_entry_120);
        cpu->register_exception_vector(121,_interrupt_entry_121);
        cpu->register_exception_vector(122,_interrupt_entry_122);
        cpu->register_exception_vector(123,_interrupt_entry_123);
        cpu->register_exception_vector(124,_interrupt_entry_124);
        cpu->register_exception_vector(125,_interrupt_entry_125);
        cpu->register_exception_vector(126,_interrupt_entry_126);
        cpu->register_exception_vector(127,_interrupt_entry_127);
    }
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
