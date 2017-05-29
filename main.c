#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#pragma pack(push, 1)
typedef struct huftable_s {
    uint32 l1; // +0
    uint16 l2; // +4
    uint32 l3; // +6
    uint16 l4; // +A
} huftable_t;

#pragma pack(pop)

typedef struct vars_s {
    uint32 calculated_crc;
    uint16 max_reps;
    uint16 enc_key;
    uint32 pack_block_size;
    uint16 dict_size;
    uint32 method;
    uint32 pu_mode;
    uint32 input_size;

    // inner
    uint32 packed_size;
    uint32 src_bytes_left;
    uint32 src_bytes_left_no_header;
    uint32 processed_size;
    uint32 v7;
    uint32 pack_block_pos;
    uint16 pack_token, bit_count, v11;
    uint16 last_min_offset;
    uint32 v17;
    uint16 v18;
    uint32 pack_block_left_size;
    uint16 match_count;
    uint16 match_offset;
    uint32 v20, v21;
    uint32 bit_buffer;

    uint32 unpacked_size;
    uint32 rnc_data_size;
    uint16 unpacked_crc;
    uint16 packed_crc;
    uint32 leeway;
    uint32 chunks_count;

    uint8 *mem1;
    uint8 *pack_block_start;
    uint8 *pack_block_max;
    uint8 *pack_block_end;
    uint16 *mem2;
    uint16 *mem3;
    uint16 *mem4;
    uint16 *mem5;

    uint8 *decoded;
    uint8 *window;

    uint32 read_start_offset, write_start_offset;
    uint8 *input, *output, *temp;
    uint32 input_offset, output_offset, temp_offset;

    uint8 tmp_crc_data[2048];
    huftable_t raw_table[16];
    huftable_t pos_table[16];
    huftable_t len_table[16];
} vars_t;

#define RNC_SIGN 0x524E43 // RNC
#define RNC_HEADER_SIZE 0x12

static const uint16 crc_table[] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static const uint8 match_count_bits_table[] = { 0x00, 0x0E, 0x08, 0x0A, 0x012, 0x013, 0x016 };
static const uint8 match_count_bits_count_table[] = { 0,  4,  4,  4,  5,  5,  5 };
static const uint8 match_offset_bits_table[] = { 0x00, 0x06, 0x08, 0x09, 0x15, 0x17, 0x1D, 0x1F, 0x28, 0x29, 0x2C, 0x2D, 0x38, 0x39, 0x3C, 0x3D };
static const uint8 match_offset_bits_count_table[] = { 1, 3, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6 };

uint8 peek_byte(uint8 *buf, uint32 offset)
{
    return buf[offset];
}

uint8 read_byte(uint8 *buf, uint32 *offset)
{
    return buf[(*offset)++];
}

void write_byte(uint8 *buf, uint32 *offset, uint8 b)
{
    buf[(*offset)++] = b;
}

uint16 peek_word_be(uint8 *buf, uint32 offset)
{
    uint8 b1 = peek_byte(buf, offset + 0);
    uint8 b2 = peek_byte(buf, offset + 1);

    return (b1 << 8) | b2;
}

uint16 read_word_be(uint8 *buf, uint32 *offset)
{
    uint8 b1 = read_byte(buf, offset);
    uint8 b2 = read_byte(buf, offset);

    return (b1 << 8) | b2;
}

void write_word_be(uint8 *buf, uint32 *offset, uint16 val)
{
    write_byte(buf, offset, (val >> 8) & 0xFF);
    write_byte(buf, offset, (val >> 0) & 0xFF);
}

uint32 peek_dword_be(uint8 *buf, uint32 offset)
{
    uint16 w1 = peek_word_be(buf, offset + 0);
    uint16 w2 = peek_word_be(buf, offset + 2);

    return (w1 << 16) | w2;
}

uint32 read_dword_be(uint8 *buf, uint32 *offset)
{
    uint16 w1 = read_word_be(buf, offset);
    uint16 w2 = read_word_be(buf, offset);

    return (w1 << 16) | w2;
}

void write_dword_be(uint8 *buf, uint32 *offset, uint32 val)
{
    write_word_be(buf, offset, (val >> 16));
    write_word_be(buf, offset, (val & 0xFFFF));
}

void read_buf(uint8 *dest, uint8 *source, uint32 *offset, int size)
{
    memcpy(dest, &source[*offset], size);
    *offset += size;
}

void write_buf(uint8 *dest, uint32 *offset, uint8 *source, int size)
{
    memcpy(&dest[*offset], source, size);
    *offset += size;
}

uint16 crc_block(uint8 *buf, uint32 offset, int size)
{
    uint16 crc = 0;

    while (size--)
    {
        crc ^= read_byte(buf, &offset);
        crc = (crc >> 8) ^ crc_table[crc & 0xFF];
    }

    return crc;
}

inline void ror_w(uint16 *x)
{
    if (*x & 1)
        *x = 0x8000 | (*x >> 1);
    else
        *x >>= 1;
}

vars_t *init_vars()
{
    vars_t *v = (vars_t*)malloc(sizeof(vars_t));
    v->enc_key = 0;
    v->max_reps = 0x1000;
    v->calculated_crc = 0;
    v->pack_block_size = 0x3000;
    v->dict_size = 0xFFFF;
    v->method = 1;
    v->pu_mode = 0;

    v->read_start_offset = 0;
    v->write_start_offset = 0;
    v->input_offset = 0;
    v->output_offset = 0;
    v->temp_offset = 0;

    memset(v->tmp_crc_data, 0, sizeof(v->tmp_crc_data));
    memset(v->raw_table, 0, sizeof(v->raw_table));
    memset(v->pos_table, 0, sizeof(v->pos_table));
    memset(v->len_table, 0, sizeof(v->len_table));

    return v;
}

int parse_args(int argc, char **argv, vars_t *vars)
{
    if (strchr("pu", argv[1][0]) || strchr("PU", argv[1][0]))
    {
        switch (argv[1][0])
        {
        case 'p': vars->pu_mode = 0; break;
        case 'u': vars->pu_mode = 1; break;
        }
    }
    else
        return 1;

    for (int i = 4; i < argc &&
        (argv[i][0] == '-' || argv[i][0] == '/'); ++i)
    {
        switch (argv[i][1])
        {
        case 'k':
            sscanf(&argv[i][3], "%hx", &vars->enc_key);
            if (!vars->enc_key)
                return 3;
            break;
        case 'd':
            sscanf(&argv[i][3], "%hu", &vars->dict_size);
            if (vars->dict_size < 0x400)
                vars->dict_size = 0x400;
            break;
        case 'i':
            sscanf(&argv[i][3], "%x", &vars->read_start_offset);
            break;
        case 'o':
            sscanf(&argv[i][3], "%x", &vars->write_start_offset);
            break;
        case 'm':
            sscanf(&argv[i][3], "%d", &vars->method);
            if (vars->method == 0 || vars->method > 2)
                return 3;
            break;
        default:
            break;
        }
    }

    return 0;
}

void init_dicts(vars_t *v)
{
    uint16 dict_size = v->dict_size;

    for (int i = 0; i < 0x800; ++i)
    {
        v->mem2[i * 0x10 + 0x0] = dict_size; v->mem2[i * 0x10 + 0x1] = dict_size;
        v->mem2[i * 0x10 + 0x2] = dict_size; v->mem2[i * 0x10 + 0x3] = dict_size;
        v->mem2[i * 0x10 + 0x4] = dict_size; v->mem2[i * 0x10 + 0x5] = dict_size;
        v->mem2[i * 0x10 + 0x6] = dict_size; v->mem2[i * 0x10 + 0x7] = dict_size;
        v->mem2[i * 0x10 + 0x8] = dict_size; v->mem2[i * 0x10 + 0x9] = dict_size;
        v->mem2[i * 0x10 + 0xA] = dict_size; v->mem2[i * 0x10 + 0xB] = dict_size;
        v->mem2[i * 0x10 + 0xC] = dict_size; v->mem2[i * 0x10 + 0xD] = dict_size;
        v->mem2[i * 0x10 + 0xE] = dict_size; v->mem2[i * 0x10 + 0xF] = dict_size;

        v->mem3[i * 0x10 + 0x0] = dict_size; v->mem3[i * 0x10 + 0x1] = dict_size;
        v->mem3[i * 0x10 + 0x2] = dict_size; v->mem3[i * 0x10 + 0x3] = dict_size;
        v->mem3[i * 0x10 + 0x4] = dict_size; v->mem3[i * 0x10 + 0x5] = dict_size;
        v->mem3[i * 0x10 + 0x6] = dict_size; v->mem3[i * 0x10 + 0x7] = dict_size;
        v->mem3[i * 0x10 + 0x8] = dict_size; v->mem3[i * 0x10 + 0x9] = dict_size;
        v->mem3[i * 0x10 + 0xA] = dict_size; v->mem3[i * 0x10 + 0xB] = dict_size;
        v->mem3[i * 0x10 + 0xC] = dict_size; v->mem3[i * 0x10 + 0xD] = dict_size;
        v->mem3[i * 0x10 + 0xE] = dict_size; v->mem3[i * 0x10 + 0xF] = dict_size;
    }

    for (int i = 0; i < dict_size; ++i)
    {
        v->mem5[i] = 0;
        v->mem4[i] = i;
    }

    v->last_min_offset = 0;
}

void update_packed_crc(vars_t *v, uint8 b)
{
    uint16 crc = v->packed_crc;
    v->packed_crc = crc_table[(crc & 0xFF) ^ b] ^ (crc >> 8);
    v->packed_size++;
}

void update_unpacked_crc(vars_t *v, uint8 b)
{
    uint16 crc = v->unpacked_crc;
    v->unpacked_crc = crc_table[(crc & 0xFF) ^ b] ^ (crc >> 8);
    v->processed_size++;
}

void write_to_output(vars_t *v, uint8 b)
{
    if (v->packed_size >= v->src_bytes_left_no_header)
        return;

    write_byte(v->output, &v->output_offset, b);
    update_packed_crc(v, b);
}

uint8 read_from_input(vars_t *v)
{
    uint8 b = read_byte(v->input, &v->input_offset);
    update_unpacked_crc(v, b);

    return b;
}

void write_bits_and_update_packed_crc_2(vars_t *v, uint16 value, int count)
{
    uint32 mask = (1 << (count - 1));

    while (count--)
    {
        v->pack_token <<= 1;

        if (value & mask)
            v->pack_token++;

        mask >>= 1;
        v->bit_count++;

        if (v->bit_count == 8)
        {
            write_to_output(v, (v->pack_token >> 0) & 0xFF);

            for (int i = 0; i < v->v11; ++i)
                write_to_output(v, v->tmp_crc_data[i]);

            v->v11 = 0;

            if ((v->processed_size > v->packed_size) && (v->processed_size - v->packed_size > v->leeway))
                v->leeway = v->processed_size - v->packed_size;

            v->bit_count = 0;
            v->pack_token = 0;
        }
    }
}

void write_bits_and_update_packed_crc_1(vars_t *v, uint16 value, int count)
{
    while (count--)
    {
        v->pack_token >>= 1;
        v->pack_token |= (value & 1) ? 0x8000 : 0;

        value >>= 1;
        v->bit_count++;

        if (v->bit_count == 16)
        {
            write_to_output(v, (v->pack_token >> 0) & 0xFF);
            write_to_output(v, (v->pack_token >> 8) & 0xFF);

            for (int i = 0; i < v->v11; ++i)
                write_to_output(v, v->tmp_crc_data[i]);

            v->v11 = 0;

            if ((v->processed_size > v->packed_size) && (v->processed_size - v->packed_size > v->leeway))
                v->leeway = v->processed_size - v->packed_size;

            v->bit_count = 0;
            v->pack_token = 0;
        }
    }
}

void write_bits_and_update_packed_crc(vars_t *v, uint16 bits, int x)
{
    if (v->method == 2)
        write_bits_and_update_packed_crc_2(v, bits, x);
    else
        write_bits_and_update_packed_crc_1(v, bits, x);
}

int find_matches(vars_t *v)
{
    v->match_count = 1;
    v->match_offset = 0;

    int match_offset = 1;
    while (match_offset < (v->pack_block_end - v->pack_block_start) && (v->pack_block_start[match_offset] == v->pack_block_start[0]))
        match_offset++;

    uint16 first_word = (v->pack_block_start[0] << 8) | (v->pack_block_start[1] << 0);
    uint16 offset = v->mem2[first_word];

    while (1)
    {
        if (offset == v->dict_size)
        {
            if ((v->match_count == 2) && (v->match_offset > 0x100))
            {
                v->match_count = 1;
                v->match_offset = 0;
            }

            break;
        }

        uint16 restore = v->mem4[offset];
        uint16 min_offset = v->last_min_offset;

        if (min_offset <= offset)
            min_offset += v->dict_size;

        min_offset -= offset;

        if ((v->pack_block_start[-min_offset] == v->pack_block_start[0]) && (v->pack_block_start[-min_offset + 1] == v->pack_block_start[1]))
        {
            uint16 max_count = v->mem5[offset];

            if (max_count <= min_offset)
            {
                if (max_count > match_offset)
                {
                    min_offset = min_offset - max_count + match_offset;
                    max_count = match_offset;
                }

                int max_size = v->pack_block_end - v->pack_block_start;
                if (max_count == match_offset)
                {
                    while (max_count < max_size && (v->pack_block_start[max_count] == v->pack_block_start[max_count - min_offset]))
                        max_count++;
                }
            }
            else
            {
                min_offset = 1;
                max_count = match_offset;
            }

            if (max_count > v->max_reps)
                max_count = v->max_reps;

            if (max_count >= v->match_count)
            {
                v->match_count = max_count;
                v->match_offset = min_offset;
            }
        }

        offset = restore;
    }

    return 0;
}

void find_and_check_matches(vars_t *v)
{
    find_matches(v);

    if (v->match_count >= 2)
    {
        if (v->pack_block_max - v->pack_block_start >= 3)
        {
            uint16 count = v->match_count;
            uint16 offset = v->match_offset;
            uint16 min_offset = v->last_min_offset;

            v->last_min_offset = (v->last_min_offset + 1) % v->dict_size;

            v->pack_block_start++;
            find_matches(v);
            v->pack_block_start--;

            v->last_min_offset = min_offset;

            if (count < v->match_count)
            {
                count = 1;
                offset = 0;
            }

            v->match_count = count;
            v->match_offset = offset;
        }
    }
}

int bits_count(int value)
{
    int count = 1;
    while (value >>= 1)
        count++;

    return count;
}

void update_bits_table(vars_t *v, huftable_t *data, uint16 bits)
{
    if (bits <= 1)
        data[bits].l1++;
    else
        data[bits_count(bits)].l1++;

    write_word_be(v->temp, &v->temp_offset, bits);
}

void encode_matches(vars_t *v, uint16 w)
{
    while (1)
    {
        uint16 restore = v->mem4[v->last_min_offset];
        v->mem4[v->last_min_offset] = v->dict_size;

        if (restore != v->last_min_offset)
        {
            uint16 buffer_word = (v->pack_block_start[-v->dict_size] << 8) | (v->pack_block_start[-v->dict_size + 1] << 0);
            v->mem2[buffer_word] = restore;

            if (v->dict_size == restore)
                v->mem3[buffer_word] = v->dict_size;
        }

        uint16 buffer_word = (v->pack_block_start[0] << 8) | (v->pack_block_start[1] << 0);

        if (v->mem2[buffer_word] == v->dict_size)
            v->mem2[buffer_word] = v->last_min_offset;
        else
            v->mem4[v->mem3[buffer_word]] = v->last_min_offset;

        v->mem3[buffer_word] = v->last_min_offset;

        int count = 1;
        while ((count < (v->pack_block_end - v->pack_block_start)) && (v->pack_block_start[count] == v->pack_block_start[0]))
            count++;

        v->mem5[v->last_min_offset] = count;

        while (1)
        {
            v->last_min_offset = (v->last_min_offset + 1) % v->dict_size;

            v->pack_block_start++;
            w--;
            count--;

            if (w == 0)
                return;

            if (count <= 1)
                break;

            v->mem5[v->last_min_offset] = count;

            if (v->last_min_offset != v->mem4[v->last_min_offset])
            {
                restore = v->mem4[v->last_min_offset];
                v->mem4[v->last_min_offset] = v->last_min_offset;

                buffer_word = (v->pack_block_start[-v->dict_size] << 8) | (v->pack_block_start[-v->dict_size + 1] << 0);

                v->mem2[buffer_word] = restore;

                if (v->dict_size == restore)
                    v->mem3[buffer_word] = v->dict_size;
            }
        }
    }
}

void proc_6(vars_t *v)
{
    v->v17 = 0;
    v->v18 = 0;
    v->pack_block_left_size = v->pack_block_size;
    v->input_offset = v->read_start_offset + v->v7 + v->pack_block_pos;
    v->temp_offset = 0;

    while (v->src_bytes_left || v->pack_block_pos)
    {
        uint16 size_to_read = 0xFFFF - v->dict_size - v->pack_block_pos;

        if (v->src_bytes_left < size_to_read)
            size_to_read = v->src_bytes_left;

        v->pack_block_start = &v->mem1[v->dict_size];
        read_buf(&v->pack_block_start[v->pack_block_pos], v->input, &v->input_offset, size_to_read);

        v->src_bytes_left -= size_to_read;
        v->pack_block_pos += size_to_read;

        v->pack_block_max = &v->pack_block_start[v->pack_block_pos];
        v->pack_block_end = &v->pack_block_start[v->pack_block_pos];

        if (v->pack_block_left_size < v->pack_block_pos)
            v->pack_block_max = &v->pack_block_start[v->pack_block_left_size];

        while ((v->pack_block_start < v->pack_block_max - 1) && v->v17 < 0xFFFE)
        {
            find_and_check_matches(v);

            if (v->match_count >= 2)
            {
                if (v->pack_block_start + v->match_count <= v->pack_block_max)
                {
                    update_bits_table(v, v->raw_table, v->v18);
                    update_bits_table(v, v->pos_table, v->match_count - 2);
                    update_bits_table(v, v->len_table, v->match_offset - 1);

                    encode_matches(v, v->match_count);
                    v->v17++;
                    v->v18 = 0;
                }
                else
                {
                    if (v->v17 != 0)
                        break;

                    v->match_count = v->pack_block_max - v->pack_block_start;
                }
            }
            else
            {
                encode_matches(v, 1);
                v->v18++;
            }
        }

        v->pack_block_pos = v->pack_block_end - v->pack_block_start;

        memcpy(&v->pack_block_start[-v->dict_size], v->mem1, v->dict_size + v->pack_block_pos);

        if ((v->pack_block_max < v->pack_block_end) || ((v->pack_block_max == v->pack_block_end) && (v->src_bytes_left == 0)) || v->v17 == 0xFFFE)
            break;

        v->pack_block_left_size -= &v->pack_block_start[-v->dict_size] - v->mem1;
    }

    if (v->pack_block_max == v->pack_block_end && v->src_bytes_left == 0 && v->v17 != 0xFFFE)
        v->v18 += v->pack_block_pos;

    update_bits_table(v, v->raw_table, v->v18);
    v->v17++;

    v->temp_offset = 0;
}

void update_tmp_crc_data(vars_t *v, uint8 b)
{
    if (v->bit_count)
    {
        v->tmp_crc_data[v->v11] = b;
        v->v11++;
    }
    else
        write_to_output(v, b);
}

void encode_matches_count(vars_t *v, uint16 w)
{
    while (w)
    {
        if (w >= 12)
        {
            if (w & 3)
            {
                write_bits_and_update_packed_crc_2(v, 0, 1);

                uint8 b = read_from_input(v);
                update_tmp_crc_data(v, (v->enc_key ^ b) & 0xFF);

                w--;
            }
            else
            {
                write_bits_and_update_packed_crc_2(v, 0x17, 5);

                if (w >= 72)
                {
                    write_bits_and_update_packed_crc_2(v, 0xF, 4);

                    for (int i = 0; i < 72; ++i)
                    {
                        uint8 b = read_from_input(v);
                        update_tmp_crc_data(v, (v->enc_key ^ b) & 0xFF);
                    }

                    w -= 72;
                }
                else
                {
                    write_bits_and_update_packed_crc_2(v, (w - 12) >> 2, 4);

                    while (w--)
                    {
                        uint8 b = read_from_input(v);
                        update_tmp_crc_data(v, (v->enc_key ^ b) & 0xFF);
                    }
                }
            }

            ror_w(&v->enc_key);
        }
        else while (w != 0)
        {
            write_bits_and_update_packed_crc_2(v, 0, 1);

            uint8 b = read_from_input(v);
            update_tmp_crc_data(v, (v->enc_key ^ b) & 0xFF);

            ror_w(&v->enc_key);

            w--;
        }
    }
}

void clear_table(huftable_t *data, int count)
{
    for (int i = 0; i < count; ++i)
    {
        data[i].l1 = 0;
        data[i].l2 = 0xFFFF;
        data[i].l3 = 0;
        data[i].l4 = 0;
    }
}

int proc_17(vars_t *v, huftable_t *data, int count)
{
    uint32 d6 = 0xFFFFFFFF;
    uint32 d5 = 0xFFFFFFFF;

    int i = 0;
    while (i < count)
    {
        if (data[i].l1)
        {
            if (data[i].l1 < d5)
            {
                d6 = d5;
                v->v21 = v->v20;
                d5 = data[i].l1;
                v->v20 = i;
            }
            else if (data[i].l1 < d6)
            {
                d6 = data[i].l1;
                v->v21 = i;
            }
        }

        i++;
    }

    return (d5 != -1 && d6 != -1);
}

uint32 inverse_bits(uint32 value, int count)
{
    int i = 0;
    while (count--)
    {
        i <<= 1;

        if (value & 1)
            i |= 1;

        value >>= 1;
    }

    return i;
}

void proc_20(huftable_t *data, int count)
{
    int val = 0;
    uint32 div = 0x80000000;
    int bits_count = 1;

    while (bits_count <= 16)
    {
        int i = 0;

        while (1)
        {
            if (i >= count)
            {
                bits_count++;
                div >>= 1;
                break;
            }

            if (data[i].l4 == bits_count)
            {
                data[i].l3 = inverse_bits(val / div, bits_count);
                val += div;
            }

            i++;
        }
    }
}

void proc_16(vars_t *v, huftable_t *data, int count)
{
    int d4 = 0;
    int ve = 0;

    for (int i = 0; i < count; ++i)
    {
        if (data[i].l1)
        {
            d4++;
            ve = i;
        }
    }

    if (d4 == 0)
        return;

    if (d4 == 1)
    {
        data[ve].l4++;
        return;
    }

    while (proc_17(v, data, count))
    {
        data[v->v20].l1 += data[v->v21].l1;
        data[v->v21].l1 = 0;
        data[v->v20].l4++;

        while (data[v->v20].l2 != 0xFFFF)
        {
            v->v20 = data[v->v20].l2;
            data[v->v20].l4++;
        }

        data[v->v20].l2 = v->v21;
        data[v->v21].l4++;

        while (data[v->v21].l2 != 0xFFFF)
        {
            v->v21 = data[v->v21].l2;
            data[v->v21].l4++;
        }
    }

    proc_20(data, count);
}

void proc_18(vars_t *v, huftable_t *data, int count)
{
    int cnt = count;

    while (cnt-- && data[cnt].l4 == 0)
        count--;

    write_bits_and_update_packed_crc_1(v, count, 5);

    for (int i = 0; i < count; ++i)
        write_bits_and_update_packed_crc_1(v, data[i].l4, 4);
}

void proc_19(vars_t *v, huftable_t *data, int count)
{
    int bits;

    if (count >= 1)
        bits = bits_count(count);
    else
        bits = count;

    write_bits_and_update_packed_crc_1(v, data[bits].l3, data[bits].l4);

    if (bits > 1)
        write_bits_and_update_packed_crc_1(v, count - (1 << (bits - 1)), bits - 1);
}

void compress_data_2(vars_t *v)
{
    int src_offset = v->read_start_offset;

    while (v->v7 < v->unpacked_size)
    {
        proc_6(v);
        v->input_offset = src_offset;

        while (v->v17--)
        {
            v->v18 = read_word_be(v->temp, &v->temp_offset);
            v->v7 += v->v18;

            encode_matches_count(v, v->v18);

            if (v->v17)
            {
                v->match_count = read_word_be(v->temp, &v->temp_offset);
                v->match_offset = read_word_be(v->temp, &v->temp_offset);

                if (v->match_count)
                {
                    if (v->match_count > 7)
                    {
                        write_bits_and_update_packed_crc_2(v, 0xF, 4);
                        update_tmp_crc_data(v, (v->match_count - 6) & 0xFF);
                    }
                    else
                        write_bits_and_update_packed_crc_2(v, match_count_bits_table[v->match_count], match_count_bits_count_table[v->match_count]);

                    write_bits_and_update_packed_crc_2(v, match_offset_bits_table[v->match_offset >> 8], match_offset_bits_count_table[v->match_offset >> 8]);
                }
                else
                    write_bits_and_update_packed_crc_2(v, 6, 3);

                update_tmp_crc_data(v, v->match_offset & 0xFF);

                v->match_count += 2;
                v->v7 += v->match_count;

                while (v->match_count--)
                    read_from_input(v);
            }
        }

        write_bits_and_update_packed_crc_2(v, 0xF, 4);
        update_tmp_crc_data(v, 0);

        if (v->v7 >= v->unpacked_size)
            write_bits_and_update_packed_crc_2(v, 0, 1);
        else
            write_bits_and_update_packed_crc_2(v, 1, 1);

        if (v->bit_count == 0)
        {
            for (int i = 0; i < v->v11; ++i)
                write_to_output(v, v->tmp_crc_data[i]);

            v->v11 = 0;
        }

        v->chunks_count++;
        src_offset = v->input_offset;
    }

    v->pack_token <<= (8 - v->bit_count);

    if (v->bit_count || v->v11)
        write_to_output(v, v->pack_token & 0xFF);
}

void compress_data_1(vars_t *v)
{
    int src_offset = v->read_start_offset;

    while (v->v7 < v->unpacked_size)
    {
        clear_table(v->len_table, _countof(v->len_table));
        clear_table(v->pos_table, _countof(v->pos_table));
        clear_table(v->raw_table, _countof(v->raw_table));

        proc_6(v);
        v->input_offset = src_offset;

        proc_16(v, v->raw_table, _countof(v->raw_table));
        proc_16(v, v->len_table, _countof(v->len_table));
        proc_16(v, v->pos_table, _countof(v->pos_table));

        proc_18(v, v->raw_table, _countof(v->raw_table));
        proc_18(v, v->len_table, _countof(v->len_table));
        proc_18(v, v->pos_table, _countof(v->pos_table));

        write_bits_and_update_packed_crc_1(v, v->v17, 16);

        while (v->v17--)
        {
            v->v18 = read_word_be(v->temp, &v->temp_offset);
            v->v7 += v->v18;

            proc_19(v, v->raw_table, v->v18);

            if (v->v18)
            {
                while (v->v18--)
                {
                    uint8 b = read_from_input(v);

                    if (v->bit_count == 0)
                        write_to_output(v, (v->enc_key ^ b) & 0xFF);
                    else
                    {
                        v->tmp_crc_data[v->v11] = (v->enc_key ^ b) & 0xFF;
                        v->v11++;
                    }
                }

                ror_w(&v->enc_key);
            }

            if (v->v17)
            {
                v->match_count = read_word_be(v->temp, &v->temp_offset);
                v->match_offset = read_word_be(v->temp, &v->temp_offset);

                proc_19(v, v->len_table, v->match_offset);
                proc_19(v, v->pos_table, v->match_count);

                v->match_count += 2;
                v->v7 += v->match_count;

                while (v->match_count--)
                    read_from_input(v);
            }
        }

        if (v->bit_count == 0)
        {
            for (int i = 0; i < v->v11; ++i)
                write_to_output(v, v->tmp_crc_data[i]);

            v->v11 = 0;
        }

        v->chunks_count++;
        src_offset = v->input_offset;
    }

    v->pack_token >>= (16 - v->bit_count);

    if (v->bit_count || v->v11)
        write_to_output(v, v->pack_token & 0xFF);

    if (v->bit_count > 8 || v->v11)
        write_to_output(v, v->pack_token >> 8);
}

void do_pack_data(vars_t *v, int in_size1, int in_size2)
{
    v->unpacked_size = in_size1;
    v->packed_size = in_size1;
    v->src_bytes_left = in_size1;

    v->src_bytes_left_no_header = (in_size2 - RNC_HEADER_SIZE);

    if (in_size1 <= RNC_HEADER_SIZE)
        return;

    v->unpacked_crc = 0;
    v->packed_crc = 0;

    v->packed_size = 0;
    v->processed_size = 0;
    v->v7 = 0;
    v->pack_block_pos = 0;
    v->pack_token = 0;
    v->bit_count = 0;
    v->v11 = 0;
    v->leeway = 0;
    v->chunks_count = 0;

    v->mem1 = (uint8 *)malloc(0xFFFF);
    v->mem2 = (uint16 *)malloc(0x10000);
    v->mem3 = (uint16 *)malloc(0x10000);
    v->mem4 = (uint16 *)malloc(0x10000);
    v->mem5 = (uint16 *)malloc(0x10000);

    init_dicts(v);

    write_dword_be(v->output, &v->output_offset, (RNC_SIGN << 8) | (v->method & 0xFF));
    write_dword_be(v->output, &v->output_offset, v->unpacked_size);
    write_dword_be(v->output, &v->output_offset, 0);
    write_word_be(v->output, &v->output_offset, 0);
    write_word_be(v->output, &v->output_offset, 0);
    write_word_be(v->output, &v->output_offset, 0);

    uint16 key = v->enc_key;
    write_bits_and_update_packed_crc(v, 0, 1); // no lock
    write_bits_and_update_packed_crc(v, (v->enc_key ? 1 : 0), 1);

    switch (v->method)
    {
    case 1: compress_data_1(v); break;
    case 2: compress_data_2(v);
    }

    for (int i = 0; i < v->v11; ++i)
        write_to_output(v, v->tmp_crc_data[i]);

    v->v11 = 0;

    v->enc_key = key;

    if (v->leeway >(v->unpacked_size - v->packed_size))
        v->leeway -= (v->unpacked_size - v->packed_size);
    else
        v->leeway = 0;

    if (v->method == 2)
        v->leeway += 2;

    v->packed_size = v->output_offset - v->write_start_offset;

    v->output_offset = v->write_start_offset + 8;

    write_dword_be(v->output, &v->output_offset, v->packed_size - RNC_HEADER_SIZE);
    write_word_be(v->output, &v->output_offset, v->unpacked_crc);
    write_word_be(v->output, &v->output_offset, v->packed_crc);

    write_byte(v->output, &v->output_offset, v->leeway);
    write_byte(v->output, &v->output_offset, v->chunks_count);

    v->output_offset = v->packed_size + v->write_start_offset;
    v->input_offset = v->unpacked_size + v->read_start_offset;

    free(v->mem1);
    free(v->mem2);
    free(v->mem3);
    free(v->mem4);
    free(v->mem5);
}

int do_pack(vars_t *v)
{
    if (v->input_size <= RNC_HEADER_SIZE)
        return 2;

    v->input_offset = 0;
    v->output_offset = 0;

    if ((peek_dword_be(v->input, v->input_offset) >> 8) == RNC_SIGN)
        return 3;

    do_pack_data(v, v->input_size, v->input_size);

    return 0;
}

uint8 read_source_byte(vars_t *v)
{
    if (v->pack_block_start == &v->mem1[0xFFFD])
    {
        int left_size = v->input_size - v->input_offset;

        int size_to_read;
        if (left_size <= 0xFFFD)
            size_to_read = left_size;
        else
            size_to_read = 0xFFFD;

        v->pack_block_start = v->mem1;

        read_buf(v->pack_block_start, v->input, &v->input_offset, size_to_read);

        if (left_size - size_to_read > 2)
            left_size = 2;

        read_buf(&v->mem1[size_to_read], v->input, &v->input_offset, left_size);
        v->input_offset -= left_size;
    }

    return *v->pack_block_start++;
}

int pack_block_start_bits_m2(vars_t *v, uint16 count)
{
    int bits = 0;

    while (count--)
    {
        if (v->bit_count == 0)
        {
            v->bit_buffer = read_source_byte(v);
            v->bit_count = 8;
        }

        bits <<= 1;

        if (v->bit_buffer & 0x80)
            bits |= 1;

        v->bit_buffer <<= 1;
        v->bit_count--;
    }

    return bits;
}

int pack_block_start_bits_m1(vars_t *v, uint16 count)
{
    int bits = 0;
    int prev_bits = 1;

    while (count--)
    {
        if (v->bit_count == 0)
        {
            uint8 b1 = read_source_byte(v);
            uint8 b2 = read_source_byte(v);
            v->bit_buffer = (v->pack_block_start[1] << 24) | (v->pack_block_start[0] << 16) | (b2 << 8) | b1;

            v->bit_count = 16;
        }

        if (v->bit_buffer & 1)
            bits |= prev_bits;

        v->bit_buffer >>= 1;
        prev_bits <<= 1;
        v->bit_count--;
    }

    return bits;
}

int pack_block_start_bits(vars_t *v, uint16 count)
{
    if (v->method != 2)
        return pack_block_start_bits_m1(v, count);
    else
        return pack_block_start_bits_m2(v, count);
}

void decode_match_count(vars_t *v)
{
    v->match_count = pack_block_start_bits_m2(v, 1) + 4;

    if (pack_block_start_bits_m2(v, 1))
        v->match_count = ((v->match_count - 1) << 1) + pack_block_start_bits_m2(v, 1);
}

void decode_match_offset(vars_t *v)
{
    v->match_offset = 0;
    if (pack_block_start_bits_m2(v, 1))
    {
        v->match_offset = pack_block_start_bits_m2(v, 1);

        if (pack_block_start_bits_m2(v, 1))
        {
            v->match_offset = ((v->match_offset << 1) | pack_block_start_bits_m2(v, 1)) | 4;

            if (pack_block_start_bits_m2(v, 1) == 0)
                v->match_offset = (v->match_offset << 1) | pack_block_start_bits_m2(v, 1);
        }
        else if (v->match_offset == 0)
            v->match_offset = pack_block_start_bits_m2(v, 1) + 2;
    }

    v->match_offset = ((v->match_offset << 8) | read_source_byte(v)) + 1;
}

void proc_9(vars_t *v)
{
    v->v18 = (pack_block_start_bits_m2(v, 4) << 2) + 12;
}

void write_decoded_byte(vars_t *v, uint8 b)
{
    if (&v->decoded[0xFFFF] == v->window)
    {
        write_buf(v->output, &v->output_offset, &v->decoded[v->dict_size], 0xFFFF - v->dict_size);
        memcpy(&v->window[-v->dict_size], v->decoded, v->dict_size);
        v->window = &v->decoded[v->dict_size];
    }

    *v->window++ = b;
    v->calculated_crc = crc_table[(v->calculated_crc ^ b) & 0xFF] ^ (v->calculated_crc >> 8);
}

int unpack_data_m2(vars_t *v)
{
    while (v->processed_size < v->input_size)
    {
        while (1)
        {
            if (pack_block_start_bits_m2(v, 1) == 0)
            {
                write_decoded_byte(v, (v->enc_key ^ read_source_byte(v)) & 0xFF);

                ror_w(&v->enc_key);

                v->processed_size++;
            }
            else
            {
                if (pack_block_start_bits_m2(v, 1))
                {
                    if (pack_block_start_bits_m2(v, 1))
                    {
                        if (pack_block_start_bits_m2(v, 1))
                        {
                            v->match_count = read_source_byte(v) + 8;

                            if (v->match_count == 8)
                            {
                                pack_block_start_bits_m2(v, 1);
                                break;
                            }
                        }
                        else
                            v->match_count = 3;

                        decode_match_offset(v);
                    }
                    else
                    {
                        v->match_count = 2;
                        v->match_offset = read_source_byte(v) + 1;
                    }

                    v->processed_size += v->match_count;

                    while (v->match_count--)
                        write_decoded_byte(v, v->window[-v->match_offset]);
                }
                else
                {
                    decode_match_count(v);

                    if (v->match_count != 9)
                    {
                        decode_match_offset(v);
                        v->processed_size += v->match_count;

                        while (v->match_count--)
                            write_decoded_byte(v, v->window[-v->match_offset]);
                    }
                    else
                    {
                        proc_9(v);
                        v->processed_size += v->v18;

                        while (v->v18--)
                            write_decoded_byte(v, (v->enc_key ^ read_source_byte(v)) & 0xFF);

                        ror_w(&v->enc_key);
                    }
                }
            }
        }
    }

    write_buf(v->output, &v->output_offset, &v->decoded[v->dict_size], v->window - &v->decoded[v->dict_size]);
    return 0;
}

void make_huftable(vars_t *v, huftable_t *data, int count)
{
    clear_table(data, count);

    int bits = pack_block_start_bits_m1(v, 5);

    if (bits)
    {
        if (bits > 16)
            bits = 16;

        for (int i = 0; i < bits; ++i)
            data[i].l4 = pack_block_start_bits_m1(v, 4);

        proc_20(data, bits);
    }
}

int proc_4(vars_t *v, huftable_t *data)
{
    int i = 0;

    while (1)
    {
        if (data[i].l4 && (data[i].l3 == (v->bit_buffer & ((1 << data[i].l4) - 1))))
        {
            pack_block_start_bits_m1(v, data[i].l4);

            if (i < 2)
                return i;

            return pack_block_start_bits_m1(v, i - 1) | (1 << (i - 1));
        }

        i++;
    }
}

int unpack_data_m1(vars_t *v)
{
    while (v->processed_size < v->input_size)
    {
        make_huftable(v, v->raw_table, _countof(v->raw_table));
        make_huftable(v, v->len_table, _countof(v->len_table));
        make_huftable(v, v->pos_table, _countof(v->pos_table));

        int counts = pack_block_start_bits_m1(v, 16);

        while (counts--)
        {
            v->v18 = proc_4(v, v->raw_table);
            v->processed_size += v->v18;

            if (v->v18)
            {
                while (v->v18--)
                    write_decoded_byte(v, (v->enc_key ^ read_source_byte(v)) & 0xFF);

                ror_w(&v->enc_key);

                v->bit_buffer = (((v->pack_block_start[2] << 16) | (v->pack_block_start[1] << 8) | v->pack_block_start[0]) << v->bit_count) | (v->bit_buffer & ((1 << v->bit_count) - 1));
            }

            if (counts)
            {
                v->match_offset = proc_4(v, v->len_table) + 1;
                v->match_count = proc_4(v, v->pos_table) + 2;
                v->processed_size += v->match_count;

                while (v->match_count--)
                    write_decoded_byte(v, v->window[-v->match_offset]);
            }
        }
    }

    write_buf(v->output, &v->output_offset, &v->decoded[v->dict_size], v->window - &v->decoded[v->dict_size]);
    return 0;
}

int do_unpack_data(vars_t *v)
{
    int start_pos = v->input_offset;

    uint32 sign = read_dword_be(v->input, &v->input_offset);
    if ((sign >> 8) != RNC_SIGN)
        return 6;

    v->method = sign & 3;
    v->input_size = read_dword_be(v->input, &v->input_offset);
    v->packed_size = read_dword_be(v->input, &v->input_offset);
    v->unpacked_crc = read_word_be(v->input, &v->input_offset);
    v->packed_crc = read_word_be(v->input, &v->input_offset);

    /*v->leeway = */read_byte(v->input, &v->input_offset);
    /*v->chunks_count = */read_byte(v->input, &v->input_offset);

    if (crc_block(v->input, v->input_offset, v->packed_size) != v->packed_crc)
        return 4;

    v->mem1 = (uint8*)malloc(0xFFFF);
    v->decoded = (uint8*)malloc(0xFFFF);
    v->pack_block_start = &v->mem1[0xFFFD];
    v->window = &v->decoded[v->dict_size];

    v->calculated_crc = 0;
    v->bit_count = 0;
    v->bit_buffer = 0;
    v->processed_size = 0;

    uint16 specified_key = v->enc_key;

    int error_code = 0;
    if (pack_block_start_bits(v, 1) && v->pu_mode == 0)
        error_code = 9;

    if (error_code == 0)
    {
        if (pack_block_start_bits(v, 1) && v->enc_key == 0) // key is needed, but not specified as argument
            error_code = 10;
    }

    if (error_code == 0)
    {
        switch (v->method)
        {
        case 1: error_code = unpack_data_m1(v); break;
        case 2: error_code = unpack_data_m2(v); break;
        }
    }

    v->enc_key = specified_key;

    free(v->mem1);
    free(v->decoded);

    v->input_offset = start_pos + v->packed_size + RNC_HEADER_SIZE;

    if (error_code)
        return error_code;

    if (v->unpacked_crc != v->calculated_crc)
        return 5;

    return 0;
}

int do_unpack(vars_t *v)
{
    v->packed_size = v->input_size;

    if (v->input_size < RNC_HEADER_SIZE)
        return 6;

    return do_unpack_data(v); // data
}

int main(int argc, char *argv[])
{
    vars_t *v = init_vars();
    if (parse_args(argc, argv, v))
        return 1;

    if (v->method == 1)
    {
        if (v->dict_size > 0x8000)
            v->dict_size = 0x8000;
        v->max_reps = 0x1000;
    }
    else if (v->method == 2)
    {
        if (v->dict_size > 0x1000)
            v->dict_size = 0x1000;
        v->max_reps = 0xFF;
    }

    FILE *in = fopen(argv[2], "rb");
    if (in == NULL)
    {
        free(v);
        return -1;
    }
    fseek(in, 0, SEEK_END);
    v->input_size = ftell(in) - v->read_start_offset;
    fseek(in, v->read_start_offset, SEEK_SET);
    v->input = (uint8*)malloc(v->input_size);
    fread(v->input, v->input_size, 1, in);
    fclose(in);

    v->output = (uint8*)malloc(0x100000);
    v->temp = (uint8*)malloc(0x100000);

    int result = 0;
    switch (v->pu_mode)
    {
    case 0: result = do_pack(v); break;
    case 1: result = do_unpack(v); break;
    }

    if (result == 0)
    {
        FILE *out = fopen(argv[3], "wb");
        if (out == NULL)
        {
            free(v->input);
            free(v->output);
            free(v->temp);
            free(v);
            return -1;
        }
        fwrite(v->output, v->output_offset, 1, out);
        fclose(out);
    }

    free(v->input);
    free(v->output);
    free(v->temp);
    free(v);

    return result;
}