clear
close all

pos = int64(-128);
dec2bin(pos)

% CANメッセージ(7バイト)に変換
pos_msg(1) = bitand(pos,255);
pos_msg(2) = bitand(pos/2^8 ,255);
pos_msg(3) = bitand(pos/2^16,255);
pos_msg(4) = bitand(pos/2^24,255);
pos_msg(5) = bitand(pos/2^32,255);
pos_msg(6) = bitand(pos/2^40,255);
pos_msg(7) = bitand(pos/2^48,255);


for i = 1:7
    dec2bin(pos_msg(i))
end


% posに変換
for i = 1:7
    if pos_msg(i) > 128
        pos_msg(i) = pos_msg(i)-256;
    end
end
pos_received = int64(bitshift(pos_msg(7), 48) + bitshift(pos_msg(6), 40) + bitshift(pos_msg(5), 32) + bitshift(pos_msg(4), 24) + bitshift(pos_msg(3), 16) + bitshift(pos_msg(2), 8) + pos_msg(1));

% Arduino -> Matlabのmulti angleの変換はこれを用いる
% for l = 1:length(pos)
%     if pos(l)  > 36028797018963967
%         pos(l) = pos(l) - 72057594037927935;
%     end
% end

dec2bin(pos_received)
