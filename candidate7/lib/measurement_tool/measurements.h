// To add a new measurement, add a new enum value to MEASUREMENT::EXP. This will
// be the handle to the measurement. Then you also have to add a tuple of the
// form (<name>, <handle>, <type>). The values mean the following:
//
// <name>: This is just a string that will be used when printing.
//
// <handle>: The handle which is used to reference the measurment in the code. This
//           has to be the same that was also added to the enum.
//
// <type>: There are serveral types of measurements.
//     * TIMER: Measures time using a start and stop function. Afterwards the sum
//              and mean can be calculated.
//     * INT: Accumulating int values. The sum and mean can be recalled from that.
//     * FLOAT: Same as INT just for float values.
//     * TIMER_DATA: Like TIMER but all the measurements are collected and thus
//                   more complex stats can be created like standard deviation.
//     * INT_DATA: Like TIMER_DATA but for INT.
//     * FLOAT_DATA: Like TIMER_DATA but for FLOAT.
//     * COUNTER: A simple counter which can just be increased.


// Add new measurements here ...
enum class MEASUREMENT::EXP {
	BBCALLS_COUNTER,
	FUT_N6_ARR,
	FUT_N6_FRECHET,

	FUT_PREPROCESSING1,
	FUT_BLACKBOX1,
	FUT_DISCSELECTION1,
	FUT_ARRANGEMENT1,
	FUT_PREPROCESSING2,
	FUT_BLACKBOX2,
	FUT_DISCSELECTION2,
	FUT_ARRANGEMENT2,
};

inline auto MEASUREMENT::getMeasurements() -> Measurements {
// ... and here
	return {
		{"black-box calls", EXP::BBCALLS_COUNTER, COUNTER},
		{"Arrangement computation of n^6 alg", EXP::FUT_N6_ARR, TIMER},
		{"Fréchet computation of n^6 alg", EXP::FUT_N6_FRECHET, TIMER},

		{"Preprocessing 1", EXP::FUT_PREPROCESSING1, TIMER},
		{"Blackbox 1", EXP::FUT_BLACKBOX1, TIMER},
		{"Disc Selection 1", EXP::FUT_DISCSELECTION1, TIMER},
		{"Arrangement 1", EXP::FUT_ARRANGEMENT1, TIMER},
		{"Preprocessing 2", EXP::FUT_PREPROCESSING2, TIMER},
		{"Blackbox 2", EXP::FUT_BLACKBOX2, TIMER},
		{"Disc Selection 2", EXP::FUT_DISCSELECTION2, TIMER},
		{"Arrangement 2", EXP::FUT_ARRANGEMENT2, TIMER},
	};
}
