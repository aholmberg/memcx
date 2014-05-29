memcx
=====
This is a toy library created as an exercise using libuv to communicate with memcached. The high-level specification:
* C++ client utilizing libuv https://github.com/joyent/libuv/tree/v0.10
* Use memcache text protocol https://github.com/memcached/memcached/blob/master/doc/protocol.txt
* Implement GET/SET operations (no special encoding or escaping)


Design Discussion
-----------------
My original ambition led me to pursue a design that could support multiple outstanding requests per connection. The theory was that highly concurrent applications could benefit from the possiblility of pipelining requests and making the client/server traffic less chatty. I think I have demonstrated superficial evidence of this theory ([further discussion in wiki](https://github.com/aholmberg/memcx/wiki/Throughput-Discussion)). 

However, there is a limitation to this approach: the protocol needs to be reliable, and state machine airtight. Without any kind of frame identifier in the message stream, an 'unexpected' state could cause the request pipeline to break. This has not been an issue in the simple commands implemented thus far, but it is recognized as a possibility.
 
Concept of Operation
--------------------
memcx is the API  
memcache_uv contains the event loop and connection pool  
requests are entered into the event loop by synchronized queue and asynchronous uv event  
each connection has a continuous read operation  
responses are joined to requests, which are queued on a connection when sent  

Known Deficiencies
------------------
* Flags and are not implemented for GET operations.
  * Flags are parsed in response, just not forwarded through API — just requires a complex type returning flags along with value
* Multi-get: does not handle get <key>* with multiple keys (I learned of this feature late, after having originally referred to an incomplete description of the protocol).
  * This can be handled by making the GetRequest::Ingest state iterative based on number of keys.

Further Improvements
--------------------
* Request batching in memcach_uv::SendNewReqests
* Characterize behavior against remote memcached (vs local)

Alternative Design
------------------
It would be intresting to compare performance of this design vs one in which there is one outstanding request per connection. One request per connection would allow more robust error handling and recovery from bad state, but may provide less overall throughput. Clients can push concurrency with more connections, but this leads to chatty network traffic and higher connection count, especially with high server->client fanout.
