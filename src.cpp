#include <windows.h>
#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <io.h>
#include <vector>

// 传感器数据结构
struct SensorData {
    std::wstring id;
    std::wstring label;
    std::wstring value;
};

std::vector<SensorData> sensorList;

// 读取 AIDA64 传感器数据
std::map<std::wstring, SensorData> ReadAida64SensorData() {
    std::map<std::wstring, SensorData> sensorDB;
    HKEY hKey;
    const wchar_t* subKey = L"Software\\FinalWire\\AIDA64\\SensorValues";

    // 打开注册表键
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        subKey,
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        std::wcerr << L"无法打开注册表键 (错误码: " << result << L")" << std::endl;
        return sensorDB;
    }

    DWORD valueCount;
    DWORD maxValueNameLen;
    DWORD maxValueDataLen;

    // 查询注册表键信息
    result = RegQueryInfoKeyW(
        hKey,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valueCount,
        &maxValueNameLen,
        &maxValueDataLen,
        nullptr,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcerr << L"注册表查询失败 (错误码: " << result << L")" << std::endl;
        return sensorDB;
    }

    // 分配缓冲区（+2 用于可能的终止符）
    wchar_t* valueName = new wchar_t[maxValueNameLen + 2];
    BYTE* valueData = new BYTE[maxValueDataLen + 2];

    for (DWORD i = 0; i < valueCount; ++i) {
        DWORD valueNameSize = maxValueNameLen + 1;
        DWORD valueDataSize = maxValueDataLen;
        DWORD valueType;

        // 枚举注册表值
        result = RegEnumValueW(
            hKey,
            i,
            valueName,
            &valueNameSize,
            nullptr,
            &valueType,
            valueData,
            &valueDataSize
        );

        if (result == ERROR_SUCCESS && valueType == REG_SZ) {
            std::wstring fullName(valueName);
            size_t dotPos = fullName.find(L'.');

            if (dotPos != std::wstring::npos) {
                std::wstring type = fullName.substr(0, dotPos);
                std::wstring id = fullName.substr(dotPos + 1);
                std::wstring data(reinterpret_cast<wchar_t*>(valueData));

                if (type == L"Label") {
                    sensorDB[id].label = data;
                }
                else if (type == L"Value") {
                    sensorDB[id].value = data;
                }
            }
        }
    }

    // 清理资源
    delete[] valueName;
    delete[] valueData;
    RegCloseKey(hKey);

    return sensorDB;
}

// 主程序
int main() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);


    // 设置采集频率（单位：秒）
    const int interval = 1;

    std::wcout << L"启动 AIDA64 传感器数据采集 (频率: " << interval << L"秒)..." << std::endl;

    while (true) {
        auto start = std::chrono::steady_clock::now();

        // 读取数据
        auto sensorData = ReadAida64SensorData();

        int datanumber = 0;
        // 显示数据
        system("cls");
        std::wcout << L"====== 传感器数据 ======" << std::endl;
        for (const auto& [id, data] : sensorData) {
            if (!data.label.empty() && !data.value.empty()) {
                // 将数据存入容器
                sensorList.push_back({ id, data.label, data.value });

                // 输出到控制台
                std::wcout << L"传感器编号: " <<  datanumber++  << " " << id
                    << L": " << data.label
                    << L" = " << data.value
                    << std::endl;
            }
        }
        
        std::wcout << std::endl << std::endl << L"用sensorList[datanumber].value调用相关的数据"
            << std::endl << L"Example:" << std::endl
            << L"传感器75: CPUcore#1的温度: sensorList[75].value = " << sensorList[75].value;

        sensorList.clear(); // 每次timer结束一定要记得清空, 否则sensorList会一直增加
        
        
        //if (!sensorList.empty()) {
        //    std::wcout << L"第一个传感器值: " << sensorList[0].value << std::endl;
        //}

        // 计算剩余等待时间
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        int sleepTime = interval * 1000 - static_cast<int>(elapsed);

        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }

    return 0;
}