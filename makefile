all:
	g++ -std=c++11 -o hospitalA hospitalA.cpp
	g++ -std=c++11 -o hospitalB hospitalB.cpp
	g++ -std=c++11 -o hospitalC hospitalC.cpp
	g++ -std=c++11 -o scheduler scheduler.cpp
	g++ -std=c++11 -o client client.cpp

clean:
	rm -f hospitalA hospitalB hospitalC scheduler client