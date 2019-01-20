
//
// TODO: Define an enumeration to represent socket state.
//
enum sockstate { CLOSED, LISTENING, CONNECTED };
enum portstate { UNBOUND, BOUND };

//
// TODO: Define a structure to represent a socket.
//

struct sock {
  int localport;
  int remoteport;
  enum sockstate state;        // socket state
  char buf[128];    
  int owner; 
  int dataAvailable;          
};

struct port{
  enum portstate state;
};
