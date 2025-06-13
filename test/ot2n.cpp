#include "emp-ot/emp-ot.h"
#include "test/test.h"
using namespace std;

// 将2选1 OT转换为n选1 OT的测试函数
template<typename T>
double test_ot2n(T* ot, NetIO* io, int party, int64_t length, int n) {
    // 为n选1 OT分配内存
    block** data = new block*[n];
    for(int i = 0; i < n; ++i) {
        data[i] = new block[length];
    }
    block* result = new block[length];
    int* choices = new int[length];
    
    // 初始化数据
    PRG prg;
    for(int i = 0; i < n; ++i) {
        prg.random_block(data[i], length);
    }
    // 随机生成选择值
    for(int64_t i = 0; i < length; ++i) {
        choices[i] = rand() % n;
    }

    auto start = clock_start();
    
    if(party == ALICE) {
        // 发送方：对每个位置，执行log(n)次2选1 OT
        for(int64_t i = 0; i < length; ++i) {
            int remaining = n;
            int current = 0;
            
            while(remaining > 1) {
                int half = remaining / 2;
                block* temp_data = new block[2];
                temp_data[0] = data[current][i];
                temp_data[1] = data[current + half][i];
                
                ot->send(temp_data, temp_data + 1, 1);
                current += half;
                remaining -= half;
                
                delete[] temp_data;
            }
        }
    } else {
        // 接收方：根据选择值执行log(n)次2选1 OT
        for(int64_t i = 0; i < length; ++i) {
            int choice = choices[i];
            int remaining = n;
            int current = 0;
            bool* b = new bool[1];
            
            while(remaining > 1) {
                int half = remaining / 2;
                b[0] = (choice >= current + half);
                
                block temp_result;
                ot->recv(&temp_result, b, 1);
                
                if(b[0]) {
                    current += half;
                }
                remaining -= half;
                
                if(remaining == 1) {
                    result[i] = temp_result;
                }
            }
            delete[] b;
        }
    }
    
    io->flush();
    long long t = time_from(start);
    
    // 验证结果
    if(party == BOB) {
        for(int64_t i = 0; i < length; ++i) {
            if(!cmpBlock(&result[i], &data[choices[i]][i], 1)) {
                error("OT2N failed!");
            }
        }
        cout << "Tests passed.\t";
    }
    
    // 清理内存
    for(int i = 0; i < n; ++i) {
        delete[] data[i];
    }
    delete[] data;
    delete[] result;
    delete[] choices;
    
    return t;
}

int main(int argc, char** argv) {
    int port, party;
    parse_party_and_port(argv, &party, &port);
    NetIO* io = new NetIO(party==ALICE ? nullptr:"127.0.0.1", port);
    
    // 设置参数
    int64_t length = 1<<20;  // 测试的OT数量
    int n = 4;               // n选1中的n值
    
    // 测试IKNP协议
    IKNP<NetIO>* iknp = new IKNP<NetIO>(io);
    cout << "IKNP 2-to-" << n << " OT\t" 
         << double(length)/test_ot2n<IKNP<NetIO>>(iknp, io, party, length, n)*1e6 
         << " OTps" << endl;
    delete iknp;
    
    // 测试Ferret协议
    FerretCOT<NetIO>* ferretcot = new FerretCOT<NetIO>(party, 1, &io, false);
    cout << "Ferret 2-to-" << n << " OT\t" 
         << double(length)/test_ot2n<FerretCOT<NetIO>>(ferretcot, io, party, length, n)*1e6 
         << " OTps" << endl;
    delete ferretcot;
    
    delete io;
    return 0;
} 