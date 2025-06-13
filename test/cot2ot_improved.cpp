#include "test/test.h"
using namespace std;

// 基于COT的改进版2选1 OT测试函数
template<typename T>
double test_cot2ot_improved(T* cot, NetIO* io, int party, int length) {
    block* data0 = new block[length];
    block* data1 = new block[length];
    bool* b = new bool[length];
    PRG prg;
    prg.random_block(data0, length);
    prg.random_block(data1, length);
    prg.random_bool(b, length);

    auto start = clock_start();
    if (party == ALICE) {
        // 发送方：使用COT发送随机数据
        block* cot_data = new block[length];
        // 保存原始数据，因为send_cot会覆盖cot_data
        block* original_data = new block[length];
        memcpy(original_data, data0, length * sizeof(block));
        
        // 设置 Delta 相关因子
        block Delta;
        prg.random_block(&Delta, 1);
        cot->Delta = Delta;
        
        cot->send_cot(cot_data, length);
        
        // 接收方会发送一个比特，表示是否需要交换顺序
        bool* need_swap = new bool[length];
        io->recv_bool(need_swap, length);
        
        // 根据need_swap调整数据顺序
        block* ot_data0 = new block[length];
        block* ot_data1 = new block[length];
        for(int i = 0; i < length; ++i) {
            if(need_swap[i]) {
                ot_data0[i] = data1[i];
                ot_data1[i] = data0[i];
            } else {
                ot_data0[i] = data0[i];
                ot_data1[i] = data1[i];
            }
        }
        
        // 使用标准OT发送调整后的数据
        cot->send(ot_data0, ot_data1, length);
        
        delete[] cot_data;
        delete[] original_data;
        delete[] need_swap;
        delete[] ot_data0;
        delete[] ot_data1;
    } else {
        // 接收方：使用COT接收随机比特和数据
        block* cot_result = new block[length];
        bool* cot_bits = new bool[length];
        cot->recv_cot(cot_result, cot_bits, length);
        
        // 计算是否需要交换顺序
        bool* need_swap = new bool[length];
        for(int i = 0; i < length; ++i) {
            need_swap[i] = (b[i] != cot_bits[i]);
        }
        
        // 发送是否需要交换的信息
        io->send_bool(need_swap, length);
        
        // 使用标准OT接收数据
        block* result = new block[length];
        cot->recv(result, b, length);
        
        // 验证结果
        for(int i = 0; i < length; ++i) {
            if(b[i]) {
                assert(memcmp(&result[i], &data1[i], sizeof(block)) == 0);
            } else {
                assert(memcmp(&result[i], &data0[i], sizeof(block)) == 0);
            }
        }

        delete[] cot_result;
        delete[] cot_bits;
        delete[] need_swap;
        delete[] result;
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

    // 测试 IKNP 协议的改进版COT转OT
    IKNP<NetIO>* iknp = new IKNP<NetIO>(io);
    cout << "IKNP Improved COT to OT\t" 
         << double(length)/test_cot2ot_improved<IKNP<NetIO>>(iknp, io, party, length)*1e6 
         << " OTps" << endl;
    delete iknp;

    // 测试 Ferret 协议的改进版COT转OT
    FerretCOT<NetIO>* ferretcot = new FerretCOT<NetIO>(party, 1, &io, false);
    cout << "Ferret Improved COT to OT\t" 
         << double(length)/test_cot2ot_improved<FerretCOT<NetIO>>(ferretcot, io, party, length)*1e6 
         << " OTps" << endl;
    delete ferretcot;

    delete io;
    return 0;
} 