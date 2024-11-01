#ifndef REQUEST_H
#define REQUEST_H


class Request {
public:
    uint8_t  opcode;
    uint32_t key;
    uint16_t index;
    size_t   value_len;
    uint8_t  ack;
    uint32_t  bitmap;
    uint32_t value[MAX_ENTRIES_PER_PACKET];
    Request(){}
    Request(uint8_t opcode, uint32_t key, uint16_t index):
                opcode(opcode), key(key), index(index)
    {
        this->value_len = 0;
        this->ack = 0;
        this->bitmap = 0;
    };
    Request(uint8_t opcode, uint32_t key, uint16_t index, size_t value_len, uint32_t *value):
                opcode(opcode), key(key), index(index), value_len(value_len)
    {
        this->ack = 0;
        this->bitmap = 0;
    };

};
static void printRequest(Request *req)
{
    printf("opcode : %u, key : %lu, index : %u value_len : %d\n",
            req->opcode, req->key, req->index, req->value_len);

}


#endif