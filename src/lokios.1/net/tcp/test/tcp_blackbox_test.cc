#include "../socket.h"
#include "../traits.h"
#include "net/mock/finterface.h"
#include "kernel/mock/fconsole.h"
#include "k++/random.h"
#include <tmock/tmock.h>

#define ALLOW_SPEC_DEVIATIONS   1

#define LOCAL_IP    ipv4::addr{1,1,1,1}
#define LOCAL_PORT  0x2222U
#define REMOTE_IP   ipv4::addr{2,2,2,2}
#define REMOTE_PORT 0x5555U
#define REMOTE_ISS  0x1234U
#define REMOTE_WS   0x4444U

using namespace tcp;

static net::finterface intf(LOCAL_IP);

struct mock_listener
{
    bool should_accept(const header* syn)
    {
        return (bool)mock("mock_listener::should_accept",syn);
    }

    void socket_accepted(tcp::socket* s)
    {
        mock("mock_listener::socket_accepted",s);
    }

    mock_listener(uint16_t port)
    {
        intf.tcp_listen(port,method_delegate(socket_accepted),
                        method_delegate(should_accept));
    }
};

struct mock_observer : public tcp::socket_observer
{
    virtual void socket_established(socket* s)
    {
        mock("mock_observer::socket_established",s);
    }

    virtual void socket_readable(socket* s)
    {
        mock("mock_observer::socket_readable",s);
    }

    virtual void socket_fin_recvd(socket* s)
    {
        mock("mock_observer::socket_fin_recvd",s);
    }

    virtual void socket_closed(socket* s)
    {
        mock("mock_observer::socket_closed",s);
    }

    virtual void socket_reset(socket* s)
    {
        mock("mock_observer::socket_reset",s);
    }
};

static mock_observer mobserver;

static uint32_t remote_snd_nxt = REMOTE_ISS;

template<typename... Args>
static void
rx_packet(Args ...args)
{
    auto* p       = intf.pop_rx_page();
    p->pay_offset = 0;

    auto* h = p->payload_cast<tcp::ipv4_tcp_headers*>();
    h->init(SIP{REMOTE_IP},DIP{intf.ip_addr},SPORT{REMOTE_PORT},
            DPORT{LOCAL_PORT},SEQ{remote_snd_nxt},WS{REMOTE_WS,0},args...);
    if (h->tcp.seq_num == remote_snd_nxt)
    {
        if (h->tcp.syn)
            ++remote_snd_nxt;
        if (h->tcp.fin)
            ++remote_snd_nxt;
    }
    p->pay_len = h->ip.total_len;

    tmock::assert_equiv(intf.handle_rx_page(p),0U);
}

template<typename... Args>
static void
_tx_expect(const char* file, unsigned int l, Args ...args)
{
    tcp::tx_op top0;
    top0.hdrs.init(SIP{LOCAL_IP},DIP{REMOTE_IP},SPORT{LOCAL_PORT},
                   DPORT{REMOTE_PORT},args...);
    top0.hdrs.ip.identification += -1;

    if (!intf.posted_ops.empty())
    {
        auto* top = static_cast<tcp::tx_op*>(intf.pop_tx_op());
        tmock::assert_mem_same(top->hdrs,top0.hdrs);
        tmock::assert_mem_same(&top->hdrs,&top0.hdrs,top0.hdrs.ip.total_len);
        intf.handle_tx_completion(top);
    }
    else
    {
        tmock::mem_dump(&top0.hdrs,top0.hdrs.ip.total_len,file,l);
        tmock::abort("Expected tx packet but no posted ops.\n",file,l);
    }
}
#define tx_expect(...) \
    _tx_expect(__builtin_FILE(),__builtin_LINE(),__VA_ARGS__)

static void
tx_expect_none(const char* file = __builtin_FILE(),
    unsigned int l = __builtin_LINE())
{
    if (!intf.posted_ops.empty())
    {
        auto* op  = klist_front(intf.posted_ops,link);
        auto* top = static_cast<tcp::tx_op*>(op);
        tmock::mem_dump(&top->hdrs,top->hdrs.ip.total_len,file,l);
        tmock::abort("Expected no tx packet but found a posted op.",file,l);
    }
}

static void
cleanup_socket(tcp::socket* s, tcp::socket::tcp_state state,
    const char* file = __builtin_FILE(), unsigned int l = __builtin_LINE())
{
    tmock::assert_equiv(s->state,state,file,l);

    while (!s->sent_send_ops.empty())
    {
        auto* sop = klist_front(s->sent_send_ops,link);
        s->sent_send_ops.pop_front();
        s->send_ops_slab.free(sop);
    }
    if (s->state == tcp::socket::TCP_CLOSED)
        intf.tcp_delete(s);
}

static tcp::socket*
transition_SYN_SENT()
{
    auto* s = intf.tcp_connect(REMOTE_IP,REMOTE_PORT,&mobserver,LOCAL_PORT);
    tx_expect(SEQ{s->iss},CTL{FSYN},WS{s->rcv_wnd,0},OPT_MSS{s->rcv_mss},
              OPT_WND_SHIFT{s->rcv_wnd_shift});
    tmock::assert_equiv(s->state,tcp::socket::TCP_SYN_SENT);
    return s;
}

static tcp::socket*
transition_SYN_RECVD()
{
    mock_listener ml(LOCAL_PORT);
    tcp::socket* s;
    texpect("mock_listener::should_accept",returns(true));
    auto e = texpect("mock_listener::socket_accepted",
                     capture(s,(uintptr_t*)&s));
    rx_packet(CTL{FSYN},OPT_MSS{1460},OPT_WND_SHIFT{0});
    tcheckpoint(e);
    s->observer = &mobserver;
    tx_expect(SEQ{s->snd_una},ACK{REMOTE_ISS+1},WS{s->rcv_wnd,0},
              CTL{FSYN|FACK},OPT_MSS{s->rcv_mss},
              OPT_WND_SHIFT{s->rcv_wnd_shift});
    tmock::assert_equiv(s->state,tcp::socket::TCP_SYN_RECVD);
    return s;
}

static tcp::socket*
transition_ESTABLISHED()
{
    auto* s = transition_SYN_SENT();
    auto e  = texpect("mock_observer::socket_established",want(s,s));
    rx_packet(ACK{s->iss+1},CTL{FSYN|FACK},OPT_MSS{1460},OPT_WND_SHIFT{0});
    tcheckpoint(e);
    tmock::assert_equiv(s->state,tcp::socket::TCP_ESTABLISHED);
    tx_expect(SEQ{s->iss+1},ACK{REMOTE_ISS+1},CTL{FACK},WS{s->rcv_wnd,0});
    return s;
}

static tcp::socket*
transition_FIN_WAIT_1()
{
    auto* s = transition_ESTABLISHED();
    s->close_send();
    tmock::assert_equiv(s->state,tcp::socket::TCP_FIN_WAIT_1);
    tx_expect(SEQ{s->iss+1},ACK{REMOTE_ISS+1},CTL{FACK|FFIN},WS{s->rcv_wnd,0});
    return s;
}

static tcp::socket*
transition_FIN_WAIT_2()
{
    auto* s = transition_FIN_WAIT_1();
    rx_packet(ACK{s->iss+2},CTL{FACK});
    tmock::assert_equiv(s->state,tcp::socket::TCP_FIN_WAIT_2);
    tx_expect_none();
    return s;
}

static tcp::socket*
transition_CLOSING()
{
    auto* s = transition_FIN_WAIT_1();
    auto e  = texpect("mock_observer::socket_fin_recvd");
    rx_packet(ACK{s->iss+1},CTL{FACK|FFIN});
    tcheckpoint(e);
    tmock::assert_equiv(s->state,tcp::socket::TCP_CLOSING);
    tx_expect(SEQ{s->iss+2},ACK{REMOTE_ISS+2},CTL{FACK},WS{s->rcv_wnd,0});
    return s;
}

static tcp::socket*
transition_TIME_WAIT()
{
    auto* s = transition_FIN_WAIT_1();
    auto e  = texpect("mock_observer::socket_fin_recvd");
    rx_packet(ACK{s->iss+2},CTL{FACK|FFIN});
    tcheckpoint(e);
    tmock::assert_equiv(s->state,tcp::socket::TCP_TIME_WAIT);
    tx_expect(SEQ{s->iss+2},ACK{REMOTE_ISS+2},CTL{FACK},WS{s->rcv_wnd,0});
    return s;
}

static tcp::socket*
transition_CLOSE_WAIT()
{
    auto* s = transition_ESTABLISHED();
    auto e  = texpect("mock_observer::socket_fin_recvd");
    rx_packet(ACK{s->iss+1},CTL{FACK|FFIN});
    tcheckpoint(e);
    tmock::assert_equiv(s->state,tcp::socket::TCP_CLOSE_WAIT);
    tx_expect(SEQ{s->iss+1},ACK{REMOTE_ISS+2},CTL{FACK},WS{s->rcv_wnd,0});
    return s;
}

static tcp::socket*
transition_LAST_ACK()
{
    auto* s = transition_CLOSE_WAIT();
    s->close_send();
    tmock::assert_equiv(s->state,tcp::socket::TCP_LAST_ACK);
    tx_expect(SEQ{s->iss+1},ACK{REMOTE_ISS+2},CTL{FACK|FFIN},WS{s->rcv_wnd,0});
    return s;
}

class tmock_test
{
    TMOCK_TEST(test_not_my_ip)
    {
        rx_packet(DIP{0,0,0,0});
        tx_expect_none();
    }

    TMOCK_TEST(test_broadcast_ip)
    {
        rx_packet(DIP{ipv4::broadcast_addr});
        tx_expect_none();
    }

    TMOCK_TEST(test_closed_rst1)
    {
        // CLOSED: An incoming segment with RST=1 is discarded.
        rx_packet(SEQ{12345},CTL{FSYN|FACK|FRST});
        tx_expect_none();
    }

    TMOCK_TEST(test_closed_ack0)
    {
        // CLOSED: An incoming segment with RST=0 causes a RST to be sent in
        //         response.  If ACK=0, SEQ=0 in the response.
        rx_packet(SEQ{REMOTE_ISS},CTL{FSYN});
        tx_expect(SEQ{0},ACK{REMOTE_ISS+1},WS{0},CTL{FRST|FACK});
    }

    TMOCK_TEST(test_closed_ack1)
    {
        // CLOSED: An incoming segment with RST=0 causes a RST to be sent in
        //         response.  If ACK=1, SEQ=SEG.ACK in the response.
        rx_packet(SEQ{REMOTE_ISS},ACK{456},CTL{FSYN|FACK});
        tx_expect(SEQ{456},CTL{FRST});
    }

    TMOCK_TEST(test_listen_reject_rst1)
    {
        // Should behave the same as test_closed_rst1.
        mock_listener ml(LOCAL_PORT);
        texpect("mock_listener::should_accept",returns(false));
        rx_packet(SEQ{12345},CTL{FSYN|FACK|FRST});
        tx_expect_none();
    }

    TMOCK_TEST(test_listen_reject_ack0)
    {
        // Should behave the same as test_closed_ack1.
        mock_listener ml(LOCAL_PORT);
        texpect("mock_listener::should_accept",returns(false));
        rx_packet(SEQ{REMOTE_ISS},CTL{FSYN});
        tx_expect(SEQ{0},ACK{REMOTE_ISS+1},WS{0},CTL{FRST|FACK});
    }

    TMOCK_TEST(test_listen_reject_ack1)
    {
        // Should behave the same as test_closed_ack1.
        mock_listener ml(LOCAL_PORT);
        texpect("mock_listener::should_accept",returns(false));
        rx_packet(SEQ{REMOTE_ISS},ACK{456},CTL{FSYN|FACK});
        tx_expect(SEQ{456},CTL{FRST});
    }

    TMOCK_TEST(test_listen_rst1)
    {
        // LISTEN: An incoming RST should be ignored.
        mock_listener ml(LOCAL_PORT);
        texpect("mock_listener::should_accept",returns(true));
        rx_packet(SEQ{12345},CTL{FSYN|FACK|FRST});
        tx_expect_none();
    }

    TMOCK_TEST(test_listen_ack1)
    {
        // LISTEN: An acceptable reset segment should be formed for any
        //         arriving ACK-bearing segment.
        mock_listener ml(LOCAL_PORT);
        texpect("mock_listener::should_accept",returns(true));
        rx_packet(SEQ{12345},ACK{456},CTL{FACK});
        tx_expect(SEQ{456},CTL{FRST});
    }

    TMOCK_TEST(test_listen_syn1)
    {
        // LISTEN: We should send a SYN/ACK segment in reply.
        mock_listener ml(LOCAL_PORT);
        tcp::socket* s;
        texpect("mock_listener::should_accept",returns(true));
        auto e = texpect("mock_listener::socket_accepted",
                         capture(s,(uintptr_t*)&s));
        rx_packet(SEQ{REMOTE_ISS},CTL{FSYN});
        tcheckpoint(e);
        tx_expect(SEQ{s->snd_una},ACK{REMOTE_ISS+1},WS{s->rcv_wnd,0},
                  CTL{FSYN|FACK},OPT_MSS{s->rcv_mss});
        cleanup_socket(s,tcp::socket::TCP_SYN_RECVD);
    }

    TMOCK_TEST(test_listen_noctl)
    {
        // LISTEN: Any other text or control value should be dropped.
        mock_listener ml(LOCAL_PORT);
        texpect("mock_listener::should_accept",returns(true));
        rx_packet(SEQ{REMOTE_ISS});
        tx_expect_none();
    }

    TMOCK_TEST(test_syn_sent_unacceptable_ack)
    {
        auto* s = transition_SYN_SENT();

        // SYN_SENT: if SEG.ACK < ISS and RST=0, send a RST.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss-1},CTL{FACK|FSYN});
        tx_expect(SEQ{s->iss-1},CTL{FRST});

        // SYN_SENT: if SEG.ACK == ISS and RST=0, send a RST.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss},CTL{FACK|FSYN});
        tx_expect(SEQ{s->iss},CTL{FRST});

        // SYN_SENT: if SEG.ACK > SND.NXT and RST=0, send a RST.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->snd_nxt+1},CTL{FACK|FSYN});
        tx_expect(SEQ{s->snd_nxt+1},CTL{FRST});

        // SYN_SENT: if SEG.ACK < ISS and RST=1, drop the packet.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss-1},CTL{FACK|FSYN|FRST});
        tx_expect_none();

        // SYN_SENT: if SEG.ACK == ISS and RST=1, drop the packet.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss},CTL{FACK|FSYN|FRST});
        tx_expect_none();

        // SYN_SENT: if SEG.ACK > SND.NXT and RST=1, drop the packet.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->snd_nxt+1},CTL{FACK|FSYN|FRST});
        tx_expect_none();

        cleanup_socket(s,tcp::socket::TCP_SYN_SENT);
    }
    
    TMOCK_TEST(test_syn_sent_ack0_rst1)
    {
        auto* s = transition_SYN_SENT();

        // SYN_SENT: if ACK=0 and RST=1, drop the segment.
        rx_packet(SEQ{REMOTE_ISS},CTL{FRST});
        tx_expect_none();

        cleanup_socket(s,tcp::socket::TCP_SYN_SENT);
    }

    TMOCK_TEST(test_syn_sent_acceptable_ack_rst1)
    {
        auto* s = transition_SYN_SENT();

        // SYN_SENT: if the ACK was acceptable and RST=1, reset the connection.
        texpect("mock_observer::socket_reset",want(s,s));
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss+1},CTL{FACK|FRST});
        tx_expect_none();

        cleanup_socket(s,tcp::socket::TCP_CLOSED);
    }

    TMOCK_TEST(test_syn_sent_ack0_syn1)
    {
        auto* s = transition_SYN_SENT();

        // SYN_SENT: if we receive a SYN but our SYN hasn't yet been ACKed,
        //           form a SYN/ACK segment and enter SYN_RECVD.
        // Note: this is the crossed-SYNs scenario.
        // ***Deviation: we handle this differently.  We've already sent the
        //               SYN and it is on the retransmit queue.  It will get
        //               re-sent if necessary.  In the meantime, we simply ACK
        //               the remote SYN.  We technically don't need to transmit
        //               a SYN/ACK but instead can transmit seperate SYN and
        //               ACK packets.
        // ***Deviation: We handle this case with our own sub-states, but the
        //               external visibility is the same.
        rx_packet(SEQ{REMOTE_ISS},CTL{FSYN});
        tx_expect(SEQ{s->iss+1},ACK{REMOTE_ISS+1},CTL{FACK},WS{s->rcv_wnd,0});

        cleanup_socket(s,tcp::socket::TCP_SYN_SENT_SYN_RECVD_WAIT_ACK);
    }

    TMOCK_TEST(test_syn_sent_acceptable_ack_syn0_rst0)
    {
        auto* s = transition_SYN_SENT();

        // SYN_SENT: "fifth, if neither of the SYN or RST bits is set then
        //           drop the segment and return".  This is after checking for
        //           an acceptable ACK.
        // ***Deviation: if the ACK was acceptable then it ACKs our SYN.  I
        //               don't think it is a requirement that we ignore this
        //               fact since SYN/ACK can be transmitted as ACK then
        //               SYN.
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss+1},CTL{FACK});
        tx_expect_none();

#if ALLOW_SPEC_DEVIATIONS
        cleanup_socket(s,tcp::socket::TCP_SYN_SENT_ACKED_WAIT_SYN);
#else
        cleanup_socket(s,tcp::socket::TCP_SYN_SENT);
#endif
    }

    TMOCK_TEST(test_syn_sent_acceptable_ack_syn1)
    {
        auto* s = transition_SYN_SENT();

        // SYN_SENT: if the ACK is acceptable, then it means the ACK number was
        //           larger than ISS (i.e. our SYN has been ACKed).  This
        //           transitions us to ESTABLISHED.  We also form an ACK
        //           segment.
        auto e = texpect("mock_observer::socket_established",want(s,s));
        rx_packet(SEQ{REMOTE_ISS},ACK{s->iss+1},CTL{FSYN|FACK});
        tcheckpoint(e);
        tx_expect(SEQ{s->iss+1},ACK{REMOTE_ISS+1},CTL{FACK},WS{s->rcv_wnd,0});

        cleanup_socket(s,tcp::socket::TCP_ESTABLISHED);
    }

    static void test_syncd_unacceptable_low_sl0_ws0(socket* s)
    {
        auto state = s->state;
        s->rcv_wnd = 0;
        rx_packet(SEQ{remote_snd_nxt-1},ACK{s->iss+1},CTL{FACK});
        tx_expect(SEQ{s->snd_nxt},ACK{s->rcv_nxt},CTL{FACK});

        cleanup_socket(s,state);
    }

    static void test_syncd_unacceptable_high_sl0_ws0(socket* s)
    {
        auto state = s->state;
        s->rcv_wnd = 0;
        rx_packet(SEQ{remote_snd_nxt+1},ACK{s->iss+1},CTL{FACK});
        tx_expect(SEQ{s->snd_nxt},ACK{s->rcv_nxt},CTL{FACK});

        cleanup_socket(s,state);
    }

    static void test_syncd_acceptable_sl0_ws0(socket* s,
            tcp::socket::tcp_state final_state)
    {
        s->rcv_wnd = 0;
        rx_packet(ACK{s->snd_nxt},CTL{FACK});
        tx_expect_none();

        cleanup_socket(s,final_state);
    }

    static void test_syncd_unacceptable_low_sl0_wsn0(socket* s)
    {
        auto state = s->state;
        TASSERT(s->rcv_wnd != 0);
        rx_packet(SEQ{remote_snd_nxt-1},ACK{s->iss+1},CTL{FACK});
        tx_expect(SEQ{s->snd_nxt},ACK{s->rcv_nxt},CTL{FACK},WS{s->rcv_wnd,0});

        cleanup_socket(s,state);
    }

    static void test_syncd_unacceptable_high_sl0_wsn0(socket* s)
    {
        auto state = s->state;
        TASSERT(s->rcv_wnd != 0);
        rx_packet(SEQ{remote_snd_nxt+s->rcv_wnd},ACK{s->iss+1},CTL{FACK});
        tx_expect(SEQ{s->snd_nxt},ACK{s->rcv_nxt},CTL{FACK},WS{s->rcv_wnd,0});

        cleanup_socket(s,state);
    }

    static void test_syncd_acceptable_low_sl0_wsn0(socket* s,
            tcp::socket::tcp_state final_state)
    {
        TASSERT(s->rcv_wnd != 0);
        rx_packet(SEQ{remote_snd_nxt},ACK{s->snd_nxt},CTL{FACK});
        tx_expect_none();

        cleanup_socket(s,final_state);
    }

    static void test_syncd_acceptable_high_sl0_wsn0(socket* s,
            tcp::socket::tcp_state final_state)
    {
        TASSERT(s->rcv_wnd != 0);
        rx_packet(SEQ{remote_snd_nxt+s->rcv_wnd-1},ACK{s->snd_nxt},CTL{FACK});
        tx_expect_none();

        cleanup_socket(s,final_state);
    }

#define TEST_UNACC_LOW_SL0_WS0(ts) \
    TMOCK_TEST(test_##ts##_unacceptable_seq_low_sl0_ws0) \
    { \
        test_syncd_unacceptable_low_sl0_ws0(transition_##ts()); \
    }
#define TEST_UNACC_HIGH_SL0_WS0(ts) \
    TMOCK_TEST(test_##ts##_unacceptable_seq_high_sl0_ws0) \
    { \
        test_syncd_unacceptable_high_sl0_ws0(transition_##ts()); \
    }
#define TEST_ACC_SL0_WS0(ts,fs,expectation) \
    TMOCK_TEST(test_##ts##_acceptable_seq_sl0_ws0) \
    { \
        auto* s = transition_##ts(); \
        if (expectation) \
            texpect(expectation,want(s,s)); \
        test_syncd_acceptable_sl0_ws0(s,tcp::socket::fs); \
    }
#define TEST_UNACC_LOW_SL0_WS1(ts) \
    TMOCK_TEST(test_##ts##_unacceptable_seq_low_sl0_wsn0) \
    { \
        test_syncd_unacceptable_low_sl0_wsn0(transition_##ts()); \
    }
#define TEST_UNACC_HIGH_SL0_WS1(ts) \
    TMOCK_TEST(test_##ts##_unacceptable_seq_high_sl0_wsn0) \
    { \
        test_syncd_unacceptable_high_sl0_wsn0(transition_##ts()); \
    }
#define TEST_ACC_LOW_SL0_WS1(ts,fs,expectation) \
    TMOCK_TEST(test_##ts##_acceptable_seq_low_sl0_wsn0) \
    { \
        auto* s = transition_##ts(); \
        if (expectation) \
            texpect(expectation,want(s,s)); \
        test_syncd_acceptable_low_sl0_wsn0(s,tcp::socket::fs); \
    }
#define TEST_ACC_HIGH_SL0_WS1(ts,fs,expectation) \
    TMOCK_TEST(test_##ts##_acceptable_seq_high_sl0_wsn0) \
    { \
        auto* s = transition_##ts(); \
        if (expectation) \
            texpect(expectation,want(s,s)); \
        test_syncd_acceptable_high_sl0_wsn0(s,tcp::socket::fs);\
    }
#define TEST_ALL_UNACC(ts) \
    TEST_UNACC_LOW_SL0_WS0(ts); \
    TEST_UNACC_HIGH_SL0_WS0(ts); \
    TEST_UNACC_LOW_SL0_WS1(ts); \
    TEST_UNACC_HIGH_SL0_WS1(ts);
#define TEST_ALL_ACC(ts,fs,e) \
    TEST_ACC_SL0_WS0(ts,fs,e); \
    TEST_ACC_LOW_SL0_WS1(ts,fs,e); \
    TEST_ACC_HIGH_SL0_WS1(ts,fs,e);
#define TEST_ALL(ts,fs,e) \
    TEST_ALL_UNACC(ts); \
    TEST_ALL_ACC(ts,fs,e)

    TEST_ALL(SYN_RECVD,TCP_ESTABLISHED,"mock_observer::socket_established");
    TEST_ALL(ESTABLISHED,TCP_ESTABLISHED,NULL);
    TEST_ALL(FIN_WAIT_1,TCP_FIN_WAIT_2,NULL);
    TEST_ALL(FIN_WAIT_2,TCP_FIN_WAIT_2,NULL);
    TEST_ALL(CLOSING,TCP_TIME_WAIT,NULL);
    TEST_ALL(TIME_WAIT,TCP_TIME_WAIT,NULL);
    TEST_ALL(CLOSE_WAIT,TCP_CLOSE_WAIT,NULL);
    TEST_ALL(LAST_ACK,TCP_CLOSED,"mock_observer::socket_closed");
};

int
main(int argc, const char* argv[])
{
    kernel::fconsole_suppress_output = true;
    return tmock::run_tests(argc,argv);
}
