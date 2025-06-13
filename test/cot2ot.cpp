#include "test/test.h"
using namespace std;

// COT 转 OT 的测试函数
template<typename T>
double test_cot2ot(T* cot, NetIO* io, int party, int length) {
    block* data0 = new block[length];
    block* data1 = new block[length];
    bool* b = new bool[length];
    PRG prg;
    prg.random_block(data0, length);
    prg.random_block(data1, length);
    prg.random_bool(b, length);

    auto start = clock_start();
    if (party == ALICE) {
        // 发送方：使用 COT 发送两个消息
        // 首先使用 COT 发送 data0
        cot->send_cot(data0, length);
        
        // 然后发送 data1 和 data0 的差值
        block* delta = new block[length];
        for(int i = 0; i < length; ++i) {
            delta[i] = data1[i] ^ data0[i];
        }
        io->send_block(delta, length);
        delete[] delta;
    } else {
        // 接收方：使用 COT 接收一个消息
        block* result = new block[length];
        cot->recv_cot(result, b, length);
        
        // 接收差值并计算最终结果
        block* delta = new block[length];
        io->recv_block(delta, length);
        
        for(int i = 0; i < length; ++i) {
            if(b[i]) {
                result[i] = result[i] ^ delta[i];
            }
        }
        
        // 验证结果
        for(int i = 0; i < length; ++i) {
            if(b[i]) {
                assert(memcmp(&result[i], &data1[i], sizeof(block)) == 0);
            } else {
                assert(memcmp(&result[i], &data0[i], sizeof(block)) == 0);
            }
        }
        delete[] result;
        delete[] delta;
    }
    double ret = time_from(start);
    delete[] data0;
    delete[] data1;
    delete[] b;
    return ret;
}

int main(int argc, char** argv) {
    int length, port, party;
    if (argc <= 3)
        length = (1<<20) + 101;
    else
        length = (1<<atoi(argv[3])) + 101;

    parse_party_and_port(argv, &party, &port);
    NetIO* io = new NetIO(party==ALICE ? nullptr:"127.0.0.1", port);

    // 测试 IKNP 协议的 COT 转 OT
    IKNP<NetIO>* iknp = new IKNP<NetIO>(io);
    cout << "IKNP COT to OT\t" << double(length)/test_cot2ot<IKNP<NetIO>>(iknp, io, party, length)*1e6 << " OTps" << endl;
    delete iknp;

    // 测试 Ferret 协议的 COT 转 OT
    FerretCOT<NetIO>* ferretcot = new FerretCOT<NetIO>(party, 1, &io, false);
    cout << "Ferret COT to OT\t" << double(length)/test_cot2ot<FerretCOT<NetIO>>(ferretcot, io, party, length)*1e6 << " OTps" << endl;
    delete ferretcot;

    delete io;
    return 0;
}