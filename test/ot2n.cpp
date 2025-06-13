#include "emp-ot/emp-ot.h"
#include "test/test.h"
using namespace std;

// 将2选1 OT转换为n选1 OT的测试函数
template<typename T>
double test_ot2n(T* ot, NetIO* io, int party, int64_t length, int n) {
    // 为n选1 OT分配内存
    block** data = new block*[n];
    for(int i = 0; i < n; ++i)
        data[i] = new block[length];
    
    // 初始化数据
    for(int i = 0; i < n; ++i)
        for(int64_t j = 0; j < length; ++j)
            data[i][j] = makeBlock(0, i);

    // 为结果分配内存
    block* result = new block[length];
    int* choices = new int[length];
    
    // 初始化选择值
    for(int64_t i = 0; i < length; ++i)
        choices[i] = rand() % n;

    // 执行n选1 OT
    if(party == ALICE) {
        // 发送方：对每个位置，执行log(n)次2选1 OT
        for(int64_t i = 0; i < length; ++i) {
            int remaining = n;
            int current = 0;
            
            while(remaining > 1) {
                int half = remaining / 2;
                block* temp_data = new block[2];
                // 修改：发送相邻的两个数据块
                temp_data[0] = data[current][i];
                temp_data[1] = data[current + 1][i];
                
                ot->send(temp_data, temp_data + 1, 1);
                current += 2;  // 修改：每次移动两个位置
                remaining -= 2;  // 修改：每次处理两个数据
                
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
                // 修改：判断是否选择第二个数据块
                b[0] = (choice == current + 1);
                
                block temp_result;
                ot->recv(&temp_result, b, 1);
                
                if(b[0]) {
                    current += 1;  // 修改：如果选择第二个，移动到第二个位置
                }
                remaining -= 2;  // 修改：每次处理两个数据
                
                if(remaining <= 1) {
                    result[i] = temp_result;
                }
            }
            delete[] b;
        }
    }

    // 验证结果
    if(party == BOB) {
        for(int64_t i = 0; i < length; ++i) {
            if(!block_cmp(&result[i], &data[choices[i]][i], 1)) {
                cout << "OT2n test failed" << endl;
                return 0;
            }
        }
    }

    // 清理内存
    for(int i = 0; i < n; ++i)
        delete[] data[i];
    delete[] data;
    delete[] result;
    delete[] choices;

    return 1;
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