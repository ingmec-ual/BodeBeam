function [ ] = plot_bode( fil )
D=load(fil);

D=sortrows(D,1);  % Ordenar por frecuencias

f=D(:,1)*1e-3;     % frecuencias (Hz)
accx=D(:,2)*1e-3;  % acc pico (m/s2)
accy=D(:,3)*1e-3;  % acc pico (m/s2)
accz=D(:,4)*1e-3;  % acc pico (m/s2)

% Aacc = Ampl*w^2 -> Ampl = Aacc / w^2
Ax = accx./((2*pi*f).^2);
Ay = accy./((2*pi*f).^2);
Az = accz./((2*pi*f).^2);

figure;
subplot(3,1,1);
semilogx(f, 20*log10(accx));
xlabel('f (Hz)'); ylabel('Amplitud (dB)'); title('Bode aceleracion X');

subplot(3,1,2);
semilogx(f, 20*log10(accy));
xlabel('f (Hz)'); ylabel('Amplitud (dB)'); title('Bode aceleracion Y');

subplot(3,1,3);
semilogx(f, 20*log10(accz));
xlabel('f (Hz)'); ylabel('Amplitud (dB)'); title('Bode aceleracion Z');

figure;
subplot(3,1,1);
semilogx(f, 20*log10(Ax));
xlabel('f (Hz)'); ylabel('Amplitud (dB)'); title('Bode amplitud X');

subplot(3,1,2);
semilogx(f, 20*log10(Ay));
xlabel('f (Hz)'); ylabel('Amplitud (dB)'); title('Bode amplitud Y');

subplot(3,1,3);
semilogx(f, 20*log10(Az));
xlabel('f (Hz)'); ylabel('Amplitud (dB)'); title('Bode amplitud Z');


end

