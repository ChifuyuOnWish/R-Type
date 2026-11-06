# Color vision filters

Open **Settings > Accessibility** to select **Off**, **Protanopia**,
**Deuteranopia** or **Tritanopia**. The preview swatches use the same
transformation as gameplay. Apply & save stores the preset and strength.
Off and 0% leave the image unchanged. Older scene tint keys are ignored.

## Method

A full-screen tint applies one color to the whole image and cannot target
the differences lost in each type of color vision deficiency. A simulation
alone reproduces some of those losses. The game instead uses an experimental
daltonization pass: the
[Machado, Oliveira and Fernandes model](https://doi.org/10.1109/TVCG.2009.113)
estimates the color difference in linear RGB. The pass redistributes the
difference into the other channels, following the idea in the
[Fidaner, Lin and Ozguven project report](https://acorn.stanford.edu/psych221/projects/2005/ofidaner/project_report.pdf).
The strength blends that adjustment with the original colors.

The background and world sprites are rendered together before applying the
transform. Player names, HUD text, and menu controls are drawn separately.
No labels are added to sprites. Existing shapes and names provide cues
independent of color. These presets are not a validated universal correction.
If the graphics backend cannot initialize the filter, gameplay stays visible
with its original colors.

## Verification

| Scenario | Expected result | Manual status |
| --- | --- | --- |
| Cycle presets in Settings | The preview swatches change immediately. | Pending |
| Campaign or Infinite with both sides firing | The filter affects the background, players, enemies and shots. | Pending |
| Strength at 0% and 100% | 0% preserves the scene; 100% shows the full adjustment. | Pending |
| Apply and save while connected | The filter updates without reconnecting; restart restores it. | Pending |
| HUD, Settings and connection screens | The menu and HUD colors remain unchanged. | Pending |
