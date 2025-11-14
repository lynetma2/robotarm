<script setup lang="ts">
import { ref } from 'vue'
import { Button } from '@/components/ui/button'
import {
  Card,
  CardContent,
  CardDescription,
  CardFooter,
  CardHeader,
  CardTitle,
} from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import {
  Tabs,
  TabsContent,
  TabsList,
  TabsTrigger,
} from '@/components/ui/tabs'
import {
  Select,
  SelectContent,
  SelectGroup,
  SelectItem,
  SelectLabel,
  SelectTrigger,
  SelectValue,
} from '@/components/ui/select'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'

// --- Refs for all our TMC2209 Settings ---

// GCONF (Global Config)
const iScaleAnalog = ref(true)
const enSpreadCycle = ref(false)
const pdnDisable = ref(true)

// IHOLD_IRUN (Currents & Delays)
const ihold = ref([16]) // Sliders use an array for their value
const irun = ref([31])
const iholddelay = ref([6])

// CHOPCONF (Chopper Config)
const mres = ref('256') // Microstep resolution
const toff = ref([4])
const tbl = ref([2])

// PWMCONF (stealthChop Config)
const pwmAutoscale = ref(true)
const pwmFreq = ref('0')
const pwmGrad = ref([100])
const pwmOfs = ref([30])

// COOLCONF (coolStep Config)
const seimin = ref(false)
const semin = ref([5])

// Timing & Thresholds
const tpowerdown = ref([20])
const tpwmthrs = ref(0) // This is a 20-bit value, so a number input is best

</script>

<template>
  <!--
    Made it wider (w-[650px]) and set a max-width.
    You can also use `w-full` if you want it to be responsive.
  -->
  <Tabs default-value="gconf" class="w-full max-w-[650px]">
    <!--
      Removed the grid layout and made the list horizontally scrollable.
      This is much cleaner for many tabs.
    -->
    <TabsList class="relative flex-nowrap overflow-x-auto">
      <TabsTrigger value="gconf">
        GCONF
      </TabsTrigger>
      <TabsTrigger value="ihold_irun">
        IHOLD_IRUN
      </TabsTrigger>
      <TabsTrigger value="chopconf">
        CHOPCONF
      </TabsTrigger>
      <TabsTrigger value="pwmconf">
        PWMCONF
      </TabsTrigger>
      <TabsTrigger value="coolconf">
        COOLCONF
      </TabsTrigger>
      <TabsTrigger value="tpowerdown">
        TPOWERDOWN
      </TabsTrigger>
      <TabsTrigger value="tpwmthrs">
        TPWMTHRS
      </TabsTrigger>
    </TabsList>

    <!-- GCONF Tab -->
    <TabsContent value="gconf">
      <Card>
        <CardHeader>
          <CardTitle>GCONF</CardTitle>
          <CardDescription>
            Global Configuration settings. These affect the general operation of the driver.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-4">
          <div class="flex items-center justify-between space-x-2">
            <Label for="iscale" class="flex flex-col space-y-1">
              <span>I_SCALE_ANALOG</span>
              <span class="font-normal leading-snug text-muted-foreground">
                Use VREF pin for current scaling.
              </span>
            </Label>
            <Switch id="iscale" :checked="iScaleAnalog" @update:checked="val => iScaleAnalog = val" />
          </div>
          <div class="flex items-center justify-between space-x-2">
            <Label for="spreadcycle" class="flex flex-col space-y-1">
              <span>EN_SPREADCYCLE</span>
              <span class="font-normal leading-snug text-muted-foreground">
                Enable spreadCycle (else stealthChop).
              </span>
            </Label>
            <Switch id="spreadcycle" :checked="enSpreadCycle" @update:checked="val => enSpreadCycle = val" />
          </div>
          <div class="flex items-center justify-between space-x-2">
            <Label for="pdn" class="flex flex-col space-y-1">
              <span>PDN_DISABLE</span>
              <span class="font-normal leading-snug text-muted-foreground">
                Disable UART standstill power down.
              </span>
            </Label>
            <Switch id="pdn" :checked="pdnDisable" @update:checked="val => pdnDisable = val" />
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save GCONF</Button>
        </CardFooter>
      </Card>
    </TabsContent>

    <!-- IHOLD_IRUN Tab -->
    <TabsContent value="ihold_irun">
      <Card>
        <CardHeader>
          <CardTitle>IHOLD_IRUN</CardTitle>
          <CardDescription>
            Settings for motor run current, hold current, and hold delay.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-6 pt-2">
          <div class="space-y-2">
            <Label for="ihold">Hold Current (IHOLD: {{ ihold[0] }}/31)</Label>
            <Slider id="ihold" v-model="ihold" :max="31" :step="1" />
          </div>
          <div class="space-y-2">
            <Label for="irun">Run Current (IRUN: {{ irun[0] }}/31)</Label>
            <Slider id="irun" v-model="irun" :max="31" :step="1" />
          </div>
          <div class="space-y-2">
            <Label for="iholddelay">Hold Delay (IHOLDDELAY: {{ iholddelay[0] }}/15)</Label>
            <Slider id="iholddelay" v-model="iholddelay" :max="15" :step="1" />
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save Currents</Button>
        </CardFooter>
      </Card>
    </TabsContent>

    <!-- CHOPCONF Tab -->
    <TabsContent value="chopconf">
      <Card>
        <CardHeader>
          <CardTitle>CHOPCONF</CardTitle>
          <CardDescription>
            Chopper configuration for spreadCycle and constant-off-time.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-6 pt-2">
          <div class="space-y-2">
            <Label for="mres">Microstep Resolution (MRES)</Label>
            <Select id="mres" v-model="mres">
              <SelectTrigger>
                <SelectValue placeholder="Select microsteps" />
              </SelectTrigger>
              <SelectContent>
                <SelectGroup>
                  <SelectItem value="256">
                    1/256
                  </SelectItem>
                  <SelectItem value="128">
                    1/128
                  </SelectItem>
                  <SelectItem value="64">
                    1/64
                  </SelectItem>
                  <SelectItem value="32">
                    1/32
                  </SelectItem>
                  <SelectItem value="16">
                    1/16
                  </SelectItem>
                  <SelectItem value="8">
                    1/8
                  </SelectItem>
                  <SelectItem value="4">
                    1/4
                  </SelectItem>
                  <SelectItem value="2">
                    1/2
                  </SelectItem>
                  <SelectItem value="1">
                    Fullstep
                  </SelectItem>
                </SelectGroup>
              </SelectContent>
            </Select>
          </div>
          <div class="space-y-2">
            <Label for="toff">Off Time (TOFF: {{ toff[0] }}/15)</Label>
            <Slider id="toff" v-model="toff" :max="15" :step="1" />
          </div>
          <div class="space-y-2">
            <Label for="tbl">Blanking Time (TBL: {{ tbl[0] }}/3)</Label>
            <Slider id="tbl" v-model="tbl" :max="3" :step="1" />
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save CHOPCONF</Button>
        </CardFooter>
      </Card>
    </TabsContent>

    <!-- PWMCONF Tab -->
    <TabsContent value="pwmconf">
      <Card>
        <CardHeader>
          <CardTitle>PWMCONF</CardTitle>
          <CardDescription>
            Configuration for stealthChop (PWM) operation.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-6 pt-2">
          <div class="flex items-center justify-between space-x-2">
            <Label for="pwm-auto" class="flex flex-col space-y-1">
              <span>PWM_AUTOSCALE</span>
              <span class="font-normal leading-snug text-muted-foreground">
                Enable automatic PWM scaling.
              </span>
            </Label>
            <Switch id="pwm-auto" :checked="pwmAutoscale" @update:checked="val => pwmAutoscale = val" />
          </div>
          <div class="space-y-2">
            <Label for="pwm-freq">PWM Frequency (PWM_FREQ)</Label>
            <Select id="pwm-freq" v-model="pwmFreq">
              <SelectTrigger>
                <SelectValue placeholder="Select frequency" />
              </SelectTrigger>
              <SelectContent>
                <SelectGroup>
                  <SelectItem value="0">
                    2/1024
                  </SelectItem>
                  <SelectItem value="1">
                    2/683
                  </SelectItem>
                  <SelectItem value="2">
                    2/512
                  </SelectItem>
                  <SelectItem value="3">
                    2/410
                  </SelectItem>
                </SelectGroup>
              </SelectContent>
            </Select>
          </div>
          <div class="space-y-2">
            <Label for="pwm-grad">PWM Gradient (PWM_GRAD: {{ pwmGrad[0] }}/255)</Label>
            <Slider id="pwm-grad" v-model="pwmGrad" :max="255" :step="1" />
          </div>
          <div class="space-y-2">
            <Label for="pwm-ofs">PWM Offset (PWM_OFS: {{ pwmOfs[0] }}/255)</Label>
            <Slider id="pwm-ofs" v-model="pwmOfs" :max="255" :step="1" />
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save PWMCONF</Button>
        </CardFooter>
      </Card>
    </TabsContent>

    <!-- COOLCONF Tab -->
    <TabsContent value="coolconf">
      <Card>
        <CardHeader>
          <CardTitle>COOLCONF</CardTitle>
          <CardDescription>
            coolStep and stallGuard configuration.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-6 pt-2">
          <div class="flex items-center justify-between space-x-2">
            <Label for="seimin" class="flex flex-col space-y-1">
              <span>SEIMIN</span>
              <span class="font-normal leading-snug text-muted-foreground">
                StallGuard min current (1/2 or 1/4).
              </span>
            </Label>
            <Switch id="seimin" :checked="seimin" @update:checked="val => seimin = val" />
          </div>
          <div class="space-y-2">
            <Label for="semin">Min StallGuard Value (SEMIN: {{ semin[0] }}/15)</Label>
            <Slider id="semin" v-model="semin" :max="15" :step="1" />
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save COOLCONF</Button>
        </CardFooter>
      </Card>
    </TabsContent>

    <!-- TPOWERDOWN Tab -->
    <TabsContent value="tpowerdown">
      <Card>
        <CardHeader>
          <CardTitle>TPOWERDOWN</CardTitle>
          <CardDescription>
            Sets the delay before the driver powers down into standstill.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-6 pt-2">
          <div class="space-y-2">
            <Label for="tpowerdown">Power Down Delay (0-255)</Label>
            <Slider id="tpowerdown" v-model="tpowerdown" :max="255" :step="1" />
            <span class="text-sm text-muted-foreground">
              Value {{ tpowerdown[0] }} (approx {{ (tpowerdown[0] * 0.175).toFixed(2) }}s)
            </span>
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save TPOWERDOWN</Button>
        </CardFooter>
      </Card>
    </TabsContent>

    <!-- TPWMTHRS Tab -->
    <TabsContent value="tpwmthrs">
      <Card>
        <CardHeader>
          <CardTitle>TPWMTHRS</CardTitle>
          <CardDescription>
            Velocity threshold for switching to stealthChop.
          </CardDescription>
        </CardHeader>
        <CardContent class="space-y-2">
          <div class="space-y-1">
            <Label for="tpwmthrs">stealthChop Threshold</Label>
            <Input id="tpwmthrs" v-model.number="tpwmthrs" type="number" placeholder="0" />
            <span class="text-sm text-muted-foreground">
              0 to 1048575. 0 = stealthChop always on.
            </span>
          </div>
        </CardContent>
        <CardFooter>
          <Button>Save TPWMTHRS</Button>
        </CardFooter>
      </Card>
    </TabsContent>
  </Tabs>
</template>
