////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <iostream>
#include <cstring>

#include "untar.hxx"

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/SGDataDistributionService.hxx>


using std::cout;
using std::cerr;
using std::endl;

using namespace simgear;

typedef struct DDSMessageData_Msg
{
  int32_t userID = -1;
  char * message = nullptr;
} DDSMessageData_Msg;

extern const dds_topic_descriptor_t DDSMessageData_Msg_desc;

#define DDSMessageData_Msg__alloc() \
((DDSMessageData_Msg*) dds_alloc (sizeof (DDSMessageData_Msg)));

#define DDSMessageData_Msg_free(d,o) \
dds_sample_free ((d), &DDSMessageData_Msg_desc, (o))

static const dds_key_descriptor_t DDSMessageData_Msg_keys[1] =
{
  { "userID", 0 }
};

static const uint32_t DDSMessageData_Msg_ops [] =
{
  DDS_OP_ADR | DDS_OP_TYPE_4BY | DDS_OP_FLAG_SGN | DDS_OP_FLAG_KEY, offsetof (DDSMessageData_Msg, userID),
  DDS_OP_ADR | DDS_OP_TYPE_STR, offsetof (DDSMessageData_Msg, message),
  DDS_OP_RTS
};

const dds_topic_descriptor_t DDSMessageData_Msg_desc =
{
  sizeof (DDSMessageData_Msg),
  sizeof (char *),
  DDS_TOPIC_FIXED_KEY | DDS_TOPIC_NO_OPTIMIZE,
  1u,
  "DDSMessageData::Msg",
  DDSMessageData_Msg_keys,
  3,
  DDSMessageData_Msg_ops,
  "<MetaData version=\"1.0.0\"><Module name=\"DDSMessageData\"><Struct name=\"Msg\"><Member name=\"userID\"><Long/></Member><Member name=\"message\"><String/></Member></Struct></Module></MetaData>"
};

void testSendReceive()
{
    DDSMessageData_Msg out = { 0, (char*)"testSendReceive" };
    DDSMessageData_Msg in;

    SG_DDS participant;

    // writer and reader store the pointer to the parsed buffers, so it knows
    // the buffer location and buffer size making it possible to call read()
    // and write() without any parameters if the buffers remains available
    // during it's lifetime.
    SG_DDS_Topic *writer = new SG_DDS_Topic(out, &DDSMessageData_Msg_desc);
    SG_DDS_Topic *reader = new SG_DDS_Topic(in, &DDSMessageData_Msg_desc);

    participant.add(writer, SG_IO_OUT);
    participant.add(reader, SG_IO_IN);

    writer->write();

    participant.wait();
    reader->read();

    SG_VERIFY(in.userID == out.userID);
    SG_VERIFY(!strcmp(in.message, out.message));
}

int main(int ac, char ** av)
{
    testSendReceive();

    std::cout << "all tests passed" << std::endl;
    return 0;
}
