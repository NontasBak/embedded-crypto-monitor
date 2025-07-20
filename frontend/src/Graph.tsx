import {
    CartesianGrid,
    Line,
    ReferenceLine,
    Scatter,
    XAxis,
    YAxis,
    ComposedChart,
} from "recharts";
import { useEffect, useState, useMemo } from "react";
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
    ChartLegend,
    ChartLegendContent,
    ChartTooltip,
    ChartTooltipContent,
} from "@/components/ui/chart";

type IndicatorType =
    | "close"
    | "sma"
    | "ema_short"
    | "ema_long"
    | "macd"
    | "signal"
    | "distance";

type CryptoData = {
    [key: string]: {
        values: number[];
        timestamps: number[];
    };
};

type ChartDataPoint = {
    timestamp: number;
    formattedTime: string;
    buySignal?: number;
    sellSignal?: number;
    [key: string]: number | string | undefined;
};

function initializeData(indicators: IndicatorType[]): CryptoData {
    const initialData: CryptoData = {};
    indicators.forEach((indicator) => {
        initialData[indicator] = {
            values: [],
            timestamps: [],
        };
    });
    return initialData;
}

const indicatorColors: Record<IndicatorType, string> = {
    close: "#8884d8",
    sma: "#82ca9d",
    ema_short: "#ffc658",
    ema_long: "#ff7c7c",
    macd: "#a48fff",
    signal: "#ff8042",
    distance: "#00c5ff",
};

const indicatorLabels: Record<IndicatorType, string> = {
    close: "Close Price",
    sma: "SMA",
    ema_short: "EMA Short",
    ema_long: "EMA Long",
    macd: "MACD",
    signal: "Signal",
    distance: "Distance",
};

const signalLabels = {
    buySignal: "Buy",
    sellSignal: "Sell",
};

function Graph({
    selectedSymbol,
    indicators = ["close"] as IndicatorType[],
    selectedWindow,
}: {
    selectedSymbol: string;
    indicators?: IndicatorType[];
    selectedWindow: string;
}) {
    const [cryptoData, setCryptoData] = useState<CryptoData>(
        initializeData(indicators),
    );

    async function fetchData(
        symbol: string,
        indicator: IndicatorType,
        window: string,
    ): Promise<{ data: { values: number[]; timestamps: number[] } }> {
        const baseUrl = "https://api-crypto-monitor.nontasbak.com";
        let url = "";

        switch (indicator) {
            case "ema_short":
                url = `${baseUrl}/ema?symbol=${symbol}&window=${window}&type=short`;
                break;
            case "ema_long":
                url = `${baseUrl}/ema?symbol=${symbol}&window=${window}&type=long`;
                break;
            default:
                url = `${baseUrl}/${indicator}?symbol=${symbol}&window=${window}`;
                break;
        }

        return await axios.get(url);
    }

    // Transform data for recharts, combine all indicators
    const chartData = useMemo((): ChartDataPoint[] => {
        const dateFormatter = new Intl.DateTimeFormat(undefined, {
            hour: "2-digit",
            minute: "2-digit",
        });
        // Find the common timestamps across all indicators
        const allTimestamps = new Set<number>();
        indicators.forEach((indicator) => {
            const indicatorData = cryptoData[indicator];
            if (indicatorData && indicatorData.timestamps.length > 0) {
                indicatorData.timestamps.forEach((ts) => allTimestamps.add(ts));
            }
        });

        const sortedTimestamps = Array.from(allTimestamps).sort(
            (a, b) => a - b,
        );

        const dataPoints = sortedTimestamps.map((timestamp) => {
            const dataPoint: ChartDataPoint = {
                timestamp,
                formattedTime: dateFormatter.format(new Date(timestamp)),
            };

            indicators.forEach((indicator) => {
                const indicatorData = cryptoData[indicator];
                if (indicatorData && indicatorData.timestamps.length > 0) {
                    const index = indicatorData.timestamps.indexOf(timestamp);
                    if (index !== -1) {
                        dataPoint[indicator] = indicatorData.values[index];
                    }
                }
            });

            return dataPoint;
        });

        // Generate buy/sell signals
        const dataPointsWithSignals = dataPoints.map((dataPoint, index) => {
            const newDataPoint = { ...dataPoint };

            // EMA crossover signals (ema_short vs ema_long)
            if (
                index > 0 &&
                dataPoint.ema_short !== undefined &&
                dataPoint.ema_long !== undefined
            ) {
                const prevPoint = dataPoints[index - 1];
                const currentEmaShort = dataPoint.ema_short as number;
                const currentEmaLong = dataPoint.ema_long as number;
                const prevEmaShort = prevPoint.ema_short as number;
                const prevEmaLong = prevPoint.ema_long as number;

                // Check for crossover
                if (
                    prevEmaShort <= prevEmaLong &&
                    currentEmaShort > currentEmaLong
                ) {
                    newDataPoint.buySignal =
                        (dataPoint.close as number) || currentEmaShort;
                } else if (
                    prevEmaShort >= prevEmaLong &&
                    currentEmaShort < currentEmaLong
                ) {
                    newDataPoint.sellSignal =
                        (dataPoint.close as number) || currentEmaShort;
                }
            }

            // MACD signals (signal vs macd)
            if (
                index > 0 &&
                dataPoint.signal !== undefined &&
                dataPoint.macd !== undefined
            ) {
                const prevPoint = dataPoints[index - 1];
                const currentSignal = dataPoint.signal as number;
                const currentMacd = dataPoint.macd as number;
                const prevSignal = prevPoint.signal as number;
                const prevMacd = prevPoint.macd as number;

                // Check for crossover
                if (prevSignal >= prevMacd && currentSignal < currentMacd) {
                    // MACD above signal (buy signal)
                    newDataPoint.buySignal =
                        (dataPoint.close as number) || currentMacd;
                } else if (
                    prevSignal <= prevMacd &&
                    currentSignal > currentMacd
                ) {
                    // Signal above MACD (sell signal)
                    newDataPoint.sellSignal =
                        (dataPoint.close as number) || currentMacd;
                }
            }

            return newDataPoint;
        });

        return dataPointsWithSignals;
    }, [cryptoData, indicators]);

    // Calculate Y-axis domain based on data range
    const yAxisDomain = useMemo(() => {
        if (chartData.length === 0) return [0, 100];

        let min = Infinity;
        let max = -Infinity;

        chartData.forEach((dataPoint) => {
            indicators.forEach((indicator) => {
                const value = dataPoint[indicator];
                if (typeof value === "number" && !isNaN(value)) {
                    min = Math.min(min, value);
                    max = Math.max(max, value);
                }
            });
        });

        if (min === Infinity || max === -Infinity) return [0, 100];

        // 5% padding to top and bottom
        const padding = (max - min) * 0.05;
        return [min - padding, max + padding];
    }, [chartData, indicators]);

    // Create dynamic chart config
    const createChartConfig = (): ChartConfig => {
        const config: ChartConfig = {};
        indicators.forEach((indicator) => {
            config[indicator] = {
                label: indicatorLabels[indicator],
                color: indicatorColors[indicator],
            };
        });

        // Add buy/sell signal configs
        config.buySignal = {
            label: "Buy",
            color: "#22c55e",
        };
        config.sellSignal = {
            label: "Sell",
            color: "#ef4444",
        };

        return config;
    };

    useEffect(() => {
        const fetchAllData = async () => {
            try {
                const promises = indicators.map(async (indicator) => {
                    try {
                        const indicatorData = await fetchData(
                            selectedSymbol,
                            indicator,
                            selectedWindow,
                        );

                        return {
                            indicator,
                            data: {
                                values: indicatorData.data.values,
                                timestamps: indicatorData.data.timestamps,
                            },
                        };
                    } catch (error) {
                        console.error(
                            `Error fetching data for ${indicator}:`,
                            error,
                        );
                    }
                });

                const results = await Promise.all(promises);

                // Filter out null results (failed requests) and build the update object
                const dataUpdates = results.reduce((acc, result) => {
                    if (result) {
                        acc[result.indicator] = result.data;
                    }
                    return acc;
                }, {} as CryptoData);
                setCryptoData(dataUpdates);
                console.log("All data fetched and set");
            } catch (error) {
                console.error("Error in fetchAllData:", error);
            }
        };

        // Reset data when symbol, indicators, or window change
        setCryptoData(initializeData(indicators));
        fetchAllData();
    }, [selectedSymbol, indicators, selectedWindow]);

    // Log chart data changes
    // useEffect(() => {
    //     console.log("Chart data for", selectedSymbol, ":", chartData);
    //     console.log("Chart data length:", chartData.length);
    //     console.log("Sample data point:", chartData[0]);
    // }, [chartData, selectedSymbol]);

    const chartConfig = createChartConfig();

    const formatYAxisTick = (value: number): string => {
        if (typeof value !== "number" || !isFinite(value)) return "0";

        const absValue = Math.abs(value);

        if (absValue > 0 && absValue < 0.001) {
            return value.toExponential(2);
        }

        if (absValue >= 1000000) {
            return value.toExponential(2);
        }

        if (absValue >= 1000) {
            return value.toFixed(1);
        } else if (absValue >= 1) {
            return value.toFixed(2);
        } else {
            return value.toFixed(4);
        }
    };

    const formatTooltipValue = (value: number): string => {
        if (typeof value !== "number" || !isFinite(value)) return "0";

        const absValue = Math.abs(value);

        if (absValue > 0 && absValue < 0.001) {
            return value.toExponential(3);
        }

        if (absValue >= 1000000) {
            return value.toExponential(3);
        }

        if (absValue >= 1000) {
            return value.toFixed(2);
        } else if (absValue >= 1) {
            return value.toFixed(4);
        } else {
            return value.toFixed(6);
        }
    };

    // Custom formatter that preserves the default tooltip appearance
    const customTooltipFormatter = (value: any, name: any, item: any) => {
        const formattedValue = formatTooltipValue(Number(value));
        let indicatorColor: string;
        let labelText: string;

        // Handle buy/sell signals
        if (name === "buySignal" || name === "sellSignal") {
            indicatorColor = name === "buySignal" ? "#22c55e" : "#ef4444";
            labelText = signalLabels[name as keyof typeof signalLabels];
        } else {
            indicatorColor =
                item?.payload?.fill ||
                item?.color ||
                indicatorColors[name as IndicatorType];
            labelText = indicatorLabels[name as IndicatorType] || name;
        }

        return (
            <>
                <div
                    className="shrink-0 rounded-[2px] h-2.5 w-2.5"
                    style={{
                        backgroundColor: indicatorColor,
                    }}
                />
                <div className="flex flex-1 justify-between leading-none items-center">
                    <span className="text-muted-foreground">{labelText}</span>
                    <span className="text-foreground font-mono font-medium tabular-nums">
                        {formattedValue}
                    </span>
                </div>
            </>
        );
    };

    return (
        <Card className="w-full">
            <CardHeader>
                <CardTitle>Crypto Analysis Chart</CardTitle>
                <CardDescription>
                    Showing {indicators.join(", ")} for {selectedSymbol}
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
                        <ComposedChart
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
                                tickLine={true}
                                axisLine={false}
                                tickMargin={8}
                                minTickGap={32}
                            />
                            <YAxis
                                width={60}
                                tickLine={false}
                                axisLine={false}
                                tickMargin={8}
                                domain={yAxisDomain}
                                tickFormatter={formatYAxisTick}
                            />
                            <ChartTooltip
                                content={
                                    <ChartTooltipContent
                                        className="w-[200px]"
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
                                        formatter={customTooltipFormatter}
                                    />
                                }
                            />
                            {indicators.map((indicator) => (
                                <Line
                                    key={indicator}
                                    dataKey={indicator}
                                    type="monotone"
                                    stroke={indicatorColors[indicator]}
                                    strokeWidth={2}
                                    dot={false}
                                    strokeOpacity={1}
                                    connectNulls={false}
                                />
                            ))}
                            {/* Buy signals */}
                            <Scatter
                                dataKey="buySignal"
                                fill="#22c55e"
                                fillOpacity={0.8}
                                stroke="#16a34a"
                                strokeWidth={2}
                                shape="circle"
                            />
                            {/* Sell signals */}
                            <Scatter
                                dataKey="sellSignal"
                                fill="#ef4444"
                                fillOpacity={0.8}
                                stroke="#dc2626"
                                strokeWidth={2}
                                shape="circle"
                            />
                            {indicators.includes("distance") && (
                                <ReferenceLine
                                    y={0}
                                    stroke="#666666"
                                    strokeDasharray="5 5"
                                    strokeOpacity={0.6}
                                />
                            )}
                            <ChartLegend content={<ChartLegendContent />} />
                        </ComposedChart>
                    </ChartContainer>
                )}
            </CardContent>
        </Card>
    );
}

export default Graph;
