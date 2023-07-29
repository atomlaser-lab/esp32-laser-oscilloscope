% ip is 192.168.1.143


u = udpport("IPV4","LocalPort",9001,"Timeout",1,"ByteOrder","little-endian");
% u = udpport("datagram","IPV4")

write(u,'test',"char","192.168.1.143",9999);

figure(10)
i=0;
while (i<1000)
    if (u.NumBytesAvailable>0)
            data = read(u,u.NumBytesAvailable,"uint16");
            tig_index = data(1);
            data(1)=[];     
            Photodiode = data(1:2:end)*3.3/2^16;
            Error = data(2:2:end)*3.3/2^16;
            clf;
            plot(Photodiode);
            hold on
            plot(Error);
            hold off
    end

i=i+1;
end
fprintf('done\n');
flush(u);
clear u
