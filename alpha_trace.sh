#!/bin/bash
echo
echo "=================================================="
echo "================Alpha INT Results================="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha DIST-INT-$i"
	./predictor traces/DIST-INT-$i
done

echo
echo "=================================================="
echo "================Alpha FP Results================="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha DIST-FP-$i"
	./predictor traces/DIST-FP-$i
done

echo
echo "=================================================="
echo "================Alpha MM Results================="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha DIST-MM-$i"
	./predictor traces/DIST-MM-$i
done

echo
echo "=================================================="
echo "================Alpha SERV Results================="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha DIST-SERV-$i"
	./predictor traces/DIST-SERV-$i
done

echo
echo "=================================================="
echo "============Alpha_Enhance INT Results============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha_Enhance DIST-INT-$i"
	./predictor_enhance traces/DIST-INT-$i
done

echo
echo "=================================================="
echo "============Alpha_Enhance FP Results=============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha_Enhance DIST-FP-$i"
	./predictor_enhance traces/DIST-FP-$i
done

echo
echo "=================================================="
echo "============Alpha_Enhance MM Results=============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha_Enhance DIST-MM-$i"
	./predictor_enhance traces/DIST-MM-$i
done

echo
echo "=================================================="
echo "===========Alpha_Enhance SERV Results============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Alpha_Enhance DIST-SERV-$i"
	./predictor_enhance traces/DIST-SERV-$i
done

echo
echo "=================================================="
echo "===========Hierarchical INT Results============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Hierarchical DIST-INT-$i"
	./predictor_comp traces/DIST-INT-$i
done

echo
echo "=================================================="
echo "===========Hierarchical FP Results============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Hierarchical DIST-FP-$i"
	./predictor_comp traces/DIST-FP-$i
done

echo
echo "=================================================="
echo "===========Hierarchical MM Results============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Hierarchical DIST-MM-$i"
	./predictor_comp traces/DIST-MM-$i
done

echo
echo "=================================================="
echo "===========Hierarchical SERV Results============="
echo "=================================================="
for i in {1..5}
do
	echo
	echo "Hierarchical DIST-SERV-$i"
	./predictor_comp traces/DIST-SERV-$i
done
