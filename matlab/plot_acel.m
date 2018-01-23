function [] = plot_acel(fil)
D=load(fil);

t=D(:,1)*0.1e-3;
t=t-t(1);
t=t/1.17; % Ts error ratio

Ts=mean(diff(t));
fs=1/Ts;
fprintf('Frecuencia de muestreo: Fs=%.03f Hz\n', fs);


accx=D(:,2)*1e-3;
accy=D(:,3)*1e-3;
accz=D(:,4)*1e-3;

fig_lim_y=10;

figure(1);
subplot(3,1,1); plot(t,accx,'.'); 
xlabel('t (s)'); ylabel('acc (m/s^2)');
title('Acc X(t)'); ylim([-1, 1]*fig_lim_y);

subplot(3,1,2); plot(t,accy,'.'); 
xlabel('t (s)'); ylabel('acc (m/s^2)');
title('Acc Y(t)'); ylim([-1, 1]*fig_lim_y);

subplot(3,1,3); plot(t,accz,'.'); 
xlabel('t (s)'); ylabel('acc (m/s^2)');
title('Acc Z(t)'); ylim([-1, 1]*fig_lim_y);

figure(2);
subplot(3,1,1); dibuja_psd(accx,fs); title('Acc X');
subplot(3,1,2); dibuja_psd(accy,fs); title('Acc Y');
subplot(3,1,3); dibuja_psd(accz,fs); title('Acc Z');

end

function [] = dibuja_psd(x,Fs)
N = length(x);

% Windowing:
w = window(@hanning,N); 
x=x.*w;
X = fft(x);
X = X(1:floor(N/2)+1);
psdx = (1/(Fs*N)) * abs(X).^2;
psdx(2:end-1) = 2*psdx(2:end-1);
freq = 0:Fs/length(x):Fs/2;

plot(freq,10*log10(psdx))
grid on
xlabel('Frecuencia (Hz)')
ylabel('PSD Potencia/frecuencia (dB/Hz)')
end
