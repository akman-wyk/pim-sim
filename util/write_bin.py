import struct
import random

pim_set_unit_test_local_memory = \
    [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  # mask 1: 256, 16

     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # mask 2: 135, 9

     0x55, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff, 0x55, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff,
     0x55, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff, 0x55, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff,  # mask 3: 192, 16

     0x55, 0x55, 0xff, 0xff, 0xff, 0xff, 0x55, 0x55, 0xff, 0xff, 0xff, 0xff, 0x55, 0x55, 0xff, 0xff,
     0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # mask 4: 120, 9

     0x2c, 0x19, 0x96, 0x56, 0xfd, 0x9a, 0x20, 0x42, 0xfd, 0xcc, 0xeb, 0x0b, 0x00, 0x00, 0x96, 0xe8,
     0x39, 0xca, 0x00, 0x00, 0x22, 0x7b, 0xbf, 0x25, 0x00, 0x00, 0x96, 0xb6, 0x25, 0xf2, 0x49, 0x7a,  # mask 5: 107, 13
     ]

pim_output_unit_test_local_memory = \
    [0x08, 0x08, 0x12, 0x28, 0x80, 0x20, 0x94, 0x04, 0x08, 0x08, 0x12, 0x28, 0x80, 0x20, 0x94, 0x04,
     0x08, 0x08, 0x12, 0x28, 0x80, 0x20, 0x94, 0x04, 0x08, 0x08, 0x12, 0x28, 0x80, 0x20, 0x94, 0x04,  # mask 1: 48

     0x00, 0x49, 0x00, 0x01, 0x08, 0x20, 0x00, 0x00, 0x00, 0x49, 0x00, 0x01, 0x08, 0x20, 0x00, 0x00,
     0x00, 0x49, 0x00, 0x01, 0x08, 0x20, 0x00, 0x00, 0x00, 0x49, 0x00, 0x01, 0x08, 0x20, 0x00, 0x00,  # mask 2: 24
     ]

pim_transfer_unit_test_local_memory = \
    [0x2c, 0x3a, 0xf0, 0xec, 0xef, 0x11, 0xf8, 0x06, 0x13, 0x28, 0x43, 0x90, 0x7f, 0x72, 0x2d, 0xfb,  # mask 1: 16Byte

     0x4c, 0xfb, 0xf3, 0x62, 0x4e, 0x11, 0xcd, 0x86, 0x45, 0x5e, 0x28, 0x61, 0xe4, 0xd3, 0x67, 0xd8,  # mask 2: 24Byte
     0xc5, 0x17, 0xf8, 0x83, 0xde, 0x56, 0x0b, 0x54,

     0xf1, 0x74, 0xeb, 0x82, 0x93, 0x11, 0xc4, 0x8d, 0x5e, 0x0e, 0x40, 0xb7, 0x28, 0x70, 0x82, 0x57,  # mask 3: 20Byte
     0xf6, 0xe2, 0x10, 0x85,

     0xf8, 0x58, 0x75, 0xe2, 0xb6, 0x6f, 0x86, 0x9a, 0x5c, 0x5f, 0xad, 0x46, 0xc4, 0x10, 0x22, 0xa8,  # mask 4: 28Byte
     0x73, 0x07, 0x3d, 0x8c, 0x47, 0xbe, 0x8f, 0x7c, 0xb9, 0xe3, 0xdd, 0x20]


def write_bin(file, byte_list):
    with open(file, 'wb') as fw:
        for x in byte_list:
            byte = struct.pack('B', x)
            fw.write(byte)


def get_rand_mask():
    rand_list = [random.randint(0, 255) for _ in range(32)]

    for i in range(3):
        index = random.randint(0, 15)
        rand_list[2 * index] = 0
        rand_list[2 * index + 1] = 0

    rand_list_hex_str = ''
    activation_element_col_cnt = 0
    for i, value in enumerate(rand_list):
        if i == 0:
            rand_list_hex_str += '['
        else:
            rand_list_hex_str += ', '
        rand_list_hex_str += hex(value)
        for j in range(8):
            if (value & (1 << j)) != 0:
                activation_element_col_cnt += 1
    rand_list_hex_str += ']'
    print(rand_list_hex_str)
    print(activation_element_col_cnt)


def get_pim_transfer_mask(valid_cnt, total_cnt, last_valid):
    mask_bit_list = [0 for _ in range(total_cnt)]
    if last_valid:
        valid_index_list = random.sample(range(0, total_cnt - 1), valid_cnt - 1)
        for valid_index in valid_index_list:
            mask_bit_list[valid_index] = 1
        mask_bit_list[-1] = 1
    else:
        valid_index_list = random.sample(range(0, total_cnt - 1), valid_cnt)
        for valid_index in valid_index_list:
            mask_bit_list[valid_index] = 1
    mask_byte_list = []
    mask_byte = 0
    for i in range(0, total_cnt):
        mask_byte = (mask_byte | (mask_bit_list[i] << (i % 8)))
        if i % 8 == 7:
            mask_byte_list.append(mask_byte)
            mask_byte = 0
    if total_cnt % 8 != 0:
        mask_byte_list.append(mask_byte)

    mask_byte_list_hex_str = ''
    for i, value in enumerate(mask_byte_list):
        if i == 0:
            mask_byte_list_hex_str += '['
        else:
            mask_byte_list_hex_str += ', '
        mask_byte_list_hex_str += hex(value)
    mask_byte_list_hex_str += ']'

    return mask_byte_list_hex_str


def get_pim_transfer_masks_hex_str():
    print(get_pim_transfer_mask(64, 128, True))
    print(get_pim_transfer_mask(96, 192, False))
    print(get_pim_transfer_mask(71, 160, True))
    print(get_pim_transfer_mask(115, 224, False))


def write_pim_set_unit_test_local_memory():
    file = r'D:\Dropbox\Dropbox\Workspace\code\pim-sim\test_data\pim_set\local_memory_image.bin'
    write_bin(file, pim_set_unit_test_local_memory)


def write_pim_output_unit_test_local_memory():
    file = r'D:\Dropbox\Dropbox\Workspace\code\pim-sim\test_data\pim_output\local_memory_image.bin'
    write_bin(file, pim_output_unit_test_local_memory)


def write_pim_transfer_unit_test_local_memory():
    file = r'D:\Dropbox\Dropbox\Workspace\code\pim-sim\test_data\pim_transfer\local_memory_image.bin'
    write_bin(file, pim_transfer_unit_test_local_memory)


if __name__ == '__main__':
    write_pim_transfer_unit_test_local_memory()