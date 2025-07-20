import { ToggleGroup, ToggleGroupItem } from "@/components/ui/toggle-group";

interface WindowSelectorProps {
    selectedWindow: string;
    onWindowChange: (window: string) => void;
}

function WindowSelector({
    selectedWindow,
    onWindowChange,
}: WindowSelectorProps) {
    return (
        <div className="flex items-center gap-2">
            <h3 className="text-sm font-medium text-muted-foreground">
                Time Window:
            </h3>
            <ToggleGroup
                type="single"
                value={selectedWindow}
                onValueChange={(value) => {
                    if (value) onWindowChange(value);
                }}
                className="justify-center"
            >
                <ToggleGroupItem value="360" aria-label="6 hours">
                    6h
                </ToggleGroupItem>
                <ToggleGroupItem value="720" aria-label="12 hours">
                    12h
                </ToggleGroupItem>
                <ToggleGroupItem value="1440" aria-label="1 day">
                    1d
                </ToggleGroupItem>
                <ToggleGroupItem value="4320" aria-label="3 days">
                    3d
                </ToggleGroupItem>
            </ToggleGroup>
        </div>
    );
}

export default WindowSelector;
