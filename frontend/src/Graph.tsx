import { CartesianGrid, Line, LineChart, XAxis, YAxis } from "recharts";
import { useEffect, useState } from "react";
import axios from "axios";

import {
    Card,
    CardContent,
    CardDescription,
    CardHeader,
    CardTitle,
} from "@/components/ui/card";
import {
    type ChartConfig,
    ChartContainer,
    ChartTooltip,
    ChartTooltipContent,
} from "@/components/ui/chart";

const SYMBOLS: string[] = [
    "BTC-USDT",
    "ADA-USDT",
    "ETH-USDT",
    "DOGE-USDT",
    "XRP-USDT",
    "SOL-USDT",
    "LTC-USDT",
    "BNB-USDT",
];

type CryptoData = {
    [key: string]: {
        values: number[];
        timestamps: number[];
    };
};

type ChartDataPoint = {
    timestamp: number;
    value: number;
    formattedTime: string;
};

function initializeData(): CryptoData {
    const initialData: CryptoData = {};
    SYMBOLS.forEach((symbol) => {
        initialData[symbol] = {
            values: [],
            timestamps: [],
        };
    });
    return initialData;
}

const chartConfig = {
    value: {
        label: "Distance",
        color: "#a48fff",
    },
} satisfies ChartConfig;

function Graph() {
    const [SMAData, setSMAData] = useState<CryptoData>(initializeData());
    const [selectedSymbol, setSelectedSymbol] = useState<string>(SYMBOLS[0]);
    const [chartData, setChartData] = useState<ChartDataPoint[]>([]);

    async function fetchData(
        symbol: string,
    ): Promise<{ data: { values: number[]; timestamps: number[] } }> {
        return await axios.get(
            `http://localhost:8080/distance?symbol=${symbol}&window=100`,
        );
    }

    const dateFormatter = new Intl.DateTimeFormat(undefined, {
        hour: "2-digit",
        minute: "2-digit",
    });

    // Transform data for recharts
    const transformDataForChart = (symbol: string): ChartDataPoint[] => {
        const symbolData = SMAData[symbol];
        if (!symbolData || symbolData.values.length === 0) {
            return [];
        }

        return symbolData.values.map((value, index) => ({
            timestamp: symbolData.timestamps[index],
            value: value,
            formattedTime: dateFormatter.format(
                new Date(symbolData.timestamps[index]),
            ),
        }));
    };

    useEffect(() => {
        const promises = SYMBOLS.map(async (symbol) => {
            try {
                const symbolData = await fetchData(symbol);
                console.log(symbolData);
                setSMAData((prevData) => ({
                    ...prevData,
                    [symbol]: {
                        values: symbolData.data.values,
                        timestamps: symbolData.data.timestamps,
                    },
                }));
            } catch (error) {
                console.error(`Error fetching data for ${symbol}:`, error);
            }
        });

        Promise.all(promises).then(() => {
            console.log("Data fetched");
        });
    }, []);

    // Update chart data when selected symbol or SMA data changes
    useEffect(() => {
        const newChartData = transformDataForChart(selectedSymbol);
        console.log("Chart data for", selectedSymbol, ":", newChartData);
        console.log("Chart data length:", newChartData.length);
        console.log("Sample data point:", newChartData[0]);
        setChartData(newChartData);
    }, [selectedSymbol, SMAData]);

    return (
        <Card className="w-full">
            <CardHeader>
                <CardTitle>Crypto Distance Chart</CardTitle>
                <CardDescription>
                    Showing distance values for {selectedSymbol} (
                    {chartData.length} data points)
                </CardDescription>
            </CardHeader>
            <CardContent>
                {chartData.length === 0 ? (
                    <div className="h-[300px] w-full flex items-center justify-center text-muted-foreground">
                        Loading chart data...
                    </div>
                ) : (
                    <ChartContainer
                        config={chartConfig}
                        className="h-[300px] w-full"
                    >
                        <LineChart
                            accessibilityLayer
                            data={chartData}
                            margin={{
                                left: 12,
                                right: 24,
                                top: 12,
                                bottom: 12,
                            }}
                        >
                            <CartesianGrid
                                vertical={true}
                                horizontal={true}
                                strokeDasharray="3 3"
                            />
                            <XAxis
                                dataKey="formattedTime"
                                tickLine={false}
                                axisLine={false}
                                tickMargin={8}
                                minTickGap={32}
                            />
                            <YAxis
                                width={50}
                                tickLine={false}
                                axisLine={false}
                                tickMargin={8}
                            />
                            <ChartTooltip
                                content={
                                    <ChartTooltipContent
                                        className="w-[150px]"
                                        nameKey="value"
                                        labelFormatter={(value) => {
                                            const dataPoint = chartData.find(
                                                (d) =>
                                                    d.formattedTime === value,
                                            );
                                            return dataPoint
                                                ? new Date(
                                                      dataPoint.timestamp,
                                                  ).toLocaleString()
                                                : value;
                                        }}
                                    />
                                }
                            />
                            <Line
                                dataKey="value"
                                type="monotone"
                                stroke="#a48fff"
                                strokeWidth={3}
                                dot={false}
                                strokeOpacity={1}
                            />
                        </LineChart>
                    </ChartContainer>
                )}
            </CardContent>
        </Card>
    );
}

export default Graph;
