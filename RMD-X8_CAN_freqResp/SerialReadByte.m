% serialread

% initial settings
clear
close all

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
set(0, 'defaultAxesFontSize', 16);
set(0, 'defaultAxesFontName', 'Times new roman');
set(0, 'defaultTextFontSize', 16);
set(0, 'defaultTextFontName', 'Times new roman');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

s = serialport("COM3", 115200);
configureTerminator(s, "CR/LF");
flush(s);
s.UserData = struct("Data", [], "Count", 1);
global Dataset countnum
configureCallback(s, "terminator", @readSerialData)

function readSerialData(src, Dataset, countnum)
    data = readline(src);
    src.UserData.Count = src.UserData.Count + 1;

    if src.UserData.Count < 20
        disp(data)
    else
        src.UserData.Data(end + 1, :) = str2num(data);
        disp(data)
    end
    if src.UserData.Count > 900
        disp("off")
        configureCallback(src, "off");
        
        Dataset = src.UserData.Data;
        time = src.UserData.Data(:, 1);
        tgt_pos = src.UserData.Data(:, 2);
        cmd_byte = src.UserData.Data(:, 3);
        temperature = src.UserData.Data(:, 4);
        cur_L = src.UserData.Data(:, 5);
        cur_H = src.UserData.Data(:, 6);
        vel_L = src.UserData.Data(:, 7);
        vel_H = src.UserData.Data(:, 8);
        pos_L = src.UserData.Data(:, 9);
        pos_H = src.UserData.Data(:, 10);
        % pos_cmd = src.UserData.Data(:, 11);
        pos_low_1 = src.UserData.Data(:, 12);
        pos_2 = src.UserData.Data(:, 13);
        pos_3 = src.UserData.Data(:, 14);
        pos_4 = src.UserData.Data(:, 15);
        pos_5 = src.UserData.Data(:, 16);
        pos_6 = src.UserData.Data(:, 17);
        pos_7 = src.UserData.Data(:, 18);

        cur = (bitshift(cur_H, 8, 'int16') + cur_L)*33/2048;
        vel = bitshift(vel_H, 8, 'int16') + vel_L;
%         pos = bitshift(pos_H, 8, 'uint16') + pos_L;
        pos = bitshift(pos_7, 48, 'int64') + bitshift(pos_6, 40, 'int64') + bitshift(pos_5, 32, 'int64') + bitshift(pos_4, 24, 'int64') + bitshift(pos_3, 16, 'int64') + bitshift(pos_2, 8, 'int64') + pos_low_1;

        countnum = src.UserData.Count;
        figure
        plot(time, cur)
        figure
        plot(time, vel)
        figure
        plot(time, tgt_pos)
        hold on
        try
            plot(time, pos)
        catch
            disp('no receibed data avairable')
        end
        
        xlabel("time [ms]")
        ylabel("input/output")
        legend({'input', 'output'})
        
        filename = ['freqRespData.mat'];
        save(filename, "Dataset")

        filename = ['freqRespFig'];        
        saveas(gcf, filename, 'png')

    end

end