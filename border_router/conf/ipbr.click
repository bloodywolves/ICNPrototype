borderrouter::IPBorderRouter
FromDevice(eth0) -> [0]borderrouter;
FromDevice(eth1)->[1]borderrouter;
out1::Queue(200)->ToDevice(eth1);
out2::Queue(200)->ToDevice(eth0);
borderrouter[1]->out1;
borderrouter[2]->out2;
borderrouter[0]->Discard;
