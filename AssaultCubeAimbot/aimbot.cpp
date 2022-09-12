// InfiniteAmmo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <Windows.h>
#include "proc.h"

int main()
{
    // Get proc ID of the target process
    DWORD procId = GetProcId(L"ac_client.exe");

    if (!procId) {
        return 1;
    }

    // Get module base address
    uintptr_t moduleBase = GetModuleBaseAddress(procId, L"ac_client.exe");


    // Get handle to process
    HANDLE hProcess = 0;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);

    bool aimbot_on = false;

    while (true) {
        if (GetAsyncKeyState(VK_RSHIFT)) {
            aimbot_on = !aimbot_on;
            if (aimbot_on) {
                std::cout << "Aimbot is enabled" << std::endl;
            }
            else {
                std::cout << "Aimbot is disabled" << std::endl;
            }
            Sleep(500);
        }

        if (aimbot_on) {
            // base address of the player pointer
            uintptr_t player_ptr = moduleBase + 0x10F4F4;

            std::vector<unsigned int> location_offsets_x = { 0x4 };
            std::vector<unsigned int> location_offsets_y = { 0x8 };
            std::vector<unsigned int> location_offsets_z = { 0xC };

            uintptr_t location_x_addr = FindDMAAddy(hProcess, player_ptr, location_offsets_x);
            uintptr_t location_y_addr = FindDMAAddy(hProcess, player_ptr, location_offsets_y);
            uintptr_t location_z_addr = FindDMAAddy(hProcess, player_ptr, location_offsets_z);

            float location_x_player = 0;
            float location_y_player = 0;
            float location_z_player = 0;

            ReadProcessMemory(hProcess, (BYTE*)location_x_addr, &location_x_player, sizeof(location_x_player), nullptr);
            ReadProcessMemory(hProcess, (BYTE*)location_y_addr, &location_y_player, sizeof(location_y_player), nullptr);
            ReadProcessMemory(hProcess, (BYTE*)location_z_addr, &location_z_player, sizeof(location_z_player), nullptr);

            // std::cout << location_x_player << " " << location_y_player << " " << location_z_player << std::endl;

            // base address of the entity pointer list
            uintptr_t entity_list_ptr = moduleBase + 0x10F4F8;

            std::vector<float> entity_distances;

            void *entity_ptr_list_temp[128] = {0};

            uintptr_t entity_list_addr = FindDMAAddy(hProcess, entity_list_ptr, {0x4});

            ReadProcessMemory(hProcess, (BYTE*)entity_list_addr, &entity_ptr_list_temp, sizeof(entity_ptr_list_temp), nullptr);

            int index = 0;

            for (int i = 0; i < sizeof(entity_ptr_list_temp); i++) {
                // std::cout << entity_ptr_list_temp[i] << std::endl;
                if (entity_ptr_list_temp[i] == 0) {
                    break;
                }
                index++;
            }

            int number_of_entities = (index - 7) / 4;

            for (int i = 0; i < number_of_entities; i++) {
                unsigned int player_offset = 0x4;
                for (int j = 0; j < i; j++) {
                    player_offset += 0x4;
                }
                std::vector<unsigned int> location_offsets_x = { player_offset, 0x4 };
                std::vector<unsigned int> location_offsets_y = { player_offset, 0x8 };
                std::vector<unsigned int> location_offsets_z = { player_offset, 0xC };

                uintptr_t location_x_addr = FindDMAAddy(hProcess, entity_list_ptr, location_offsets_x);
                uintptr_t location_y_addr = FindDMAAddy(hProcess, entity_list_ptr, location_offsets_y);
                uintptr_t location_z_addr = FindDMAAddy(hProcess, entity_list_ptr, location_offsets_z);

                float location_x = 0;
                float location_y = 0;
                float location_z = 0;

                ReadProcessMemory(hProcess, (BYTE*)location_x_addr, &location_x, sizeof(location_x), nullptr);
                ReadProcessMemory(hProcess, (BYTE*)location_y_addr, &location_y, sizeof(location_y), nullptr);
                ReadProcessMemory(hProcess, (BYTE*)location_z_addr, &location_z, sizeof(location_z), nullptr);

                // std::cout << location_x << " " << location_y << " " << location_z << std::endl;

                entity_distances.push_back(sqrt(pow((location_x - location_x_player), 2) + pow((location_y - location_y_player), 2) + pow((location_z - location_z_player), 2)));
            }

            int lowest = 0;
            for (int i = 0; i < number_of_entities; i++) {
                // std::cout << entity_distances[i] << std::endl;
                if (entity_distances[lowest] > entity_distances[i]) {
                    lowest = i;
                }
            }

            unsigned int player_offset = 0x4;
            for (int i = 0; i < lowest; i++) {
                player_offset += 0x4;
            }

            // std::cout << lowest << std::endl;

            std::vector<unsigned int> entity_name_offsets = { player_offset, 0x225 };
            uintptr_t entity_name_ptr = FindDMAAddy(hProcess, entity_list_ptr, entity_name_offsets);

            char name[16] = {};

            ReadProcessMemory(hProcess, (BYTE*)entity_name_ptr, &name, sizeof(name), nullptr);

            // std::cout << "Closest to entity: " << name << std::endl;

            std::vector<unsigned int> target_location_offsets_x = { player_offset, 0x4 };
            std::vector<unsigned int> target_location_offsets_y = { player_offset, 0x8 };
            std::vector<unsigned int> target_location_offsets_z = { player_offset, 0xC };

            uintptr_t target_location_x_addr = FindDMAAddy(hProcess, entity_list_ptr, target_location_offsets_x);
            uintptr_t target_location_y_addr = FindDMAAddy(hProcess, entity_list_ptr, target_location_offsets_y);
            uintptr_t target_location_z_addr = FindDMAAddy(hProcess, entity_list_ptr, target_location_offsets_z);

            float target_location_x = 0;
            float target_location_y = 0;
            float target_location_z = 0;

            ReadProcessMemory(hProcess, (BYTE*)target_location_x_addr, &target_location_x, sizeof(target_location_x), nullptr);
            ReadProcessMemory(hProcess, (BYTE*)target_location_y_addr, &target_location_y, sizeof(target_location_y), nullptr);
            ReadProcessMemory(hProcess, (BYTE*)target_location_z_addr, &target_location_z, sizeof(target_location_z), nullptr);

            float new_view_x = target_location_x - location_x_player;
            float new_view_y = target_location_y - location_y_player;
            float new_view_z = target_location_z - location_z_player;

            // std::cout << new_view_x << " " << new_view_y << " " << new_view_z << std::endl;

            new_view_x /= sqrt(pow((target_location_x - location_x_player), 2) + pow((target_location_y - location_y_player), 2) + pow((target_location_z - location_z_player), 2));
            new_view_y /= sqrt(pow((target_location_x - location_x_player), 2) + pow((target_location_y - location_y_player), 2) + pow((target_location_z - location_z_player), 2));
            new_view_z /= sqrt(pow((target_location_x - location_x_player), 2) + pow((target_location_y - location_y_player), 2) + pow((target_location_z - location_z_player), 2));

            // std::cout << new_view_x << " " << new_view_y << " " << new_view_z << std::endl;

            float new_yaw = ((atan(new_view_y / new_view_x) / (2 * acos(0.0))) * 180) + 90;

            if (new_view_x < 0) {
                new_yaw = new_yaw - 180;
            }

            // std::cout << new_yaw << std::endl;

            std::vector<unsigned int> yaw_offsets = { 0x40 };
            std::vector<unsigned int> pitch_offsets = { 0x44 };

            uintptr_t yaw_addr = FindDMAAddy(hProcess, player_ptr, yaw_offsets);
            uintptr_t pitch_addr = FindDMAAddy(hProcess, player_ptr, pitch_offsets);


            WriteProcessMemory(hProcess, (BYTE*)yaw_addr, &new_yaw, sizeof(new_yaw), nullptr);

            entity_distances.clear();
        }
    }

    // getchar();



    return 0;
}