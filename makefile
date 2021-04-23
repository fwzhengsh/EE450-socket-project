all:
	g++ -o hospitalA hospitalA.cpp
	g++ -o hospitalB hospitalB.cpp
	g++ -o hospitalC hospitalC.cpp
	g++ -o scheduler scheduler.cpp
	g++ -o client client.cpp

clean:
	rm -f hospitalA hospitalB hospitalC scheduler client