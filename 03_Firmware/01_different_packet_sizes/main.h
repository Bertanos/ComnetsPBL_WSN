/*
 * In this task we only have two classes of nodes; the root which receives data, and the sensor nodes which send sensor data to the root node.
 */
typedef enum
{
  WSN_UNSET_ROLE,
  WSN_ROOT_ROLE,
  WSN_SENSOR_ROLE,
  WSN_ROLE_MAX
} WSN_Role_e;

/*
 * This enum defines our Inter Process Communication message types. These are used by RIOT processes (https://doc.riot-os.org/group__core__msg.html) to talk to one another. 
 * 
 * Here we use them as event triggers in our single process. Add new ones as needed.
 */
typedef enum
{
  WSN_IPC_START = 0xfff0, // To not overlap with RIOT's own IPC types we start ours from 0xfff0
  WSN_IPC_PERIODIC_OPERATION,
  /* Fill this in as needed ... */
  WS_IPC_MAX
} WSN_IPC_e;

/*
* Below defines the datastructure that our nodes produce and transmit. Change as needed. 
*/
typedef struct 
{
  uint8_t payloadSize; // Bytes
  uint8_t payload[];
} __attribute__((packed)) WSN_UDP_Pkt_t;
