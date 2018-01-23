function [ t, accx, accy, accz ] = load_acel( nombre_fichero )
%LOAD_ACEL Carga un fichero de datos capturado con la practica BodeBeam
% y devuelve los vectores de tiempos en segundos y aceleraciones X,Y,Z 
% en m/s^2.

D=load(nombre_fichero);

t=D(:,1)*0.1e-3;
t=t-t(1);
t=t/1.17; % Ts error ratio

accx=D(:,2)*1e-3;
accy=D(:,3)*1e-3;
accz=D(:,4)*1e-3;
end

