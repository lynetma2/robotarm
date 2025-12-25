<script setup lang="ts">
import { Zap, ZapOff, Activity, Gauge, Cpu, RotateCcw } from 'lucide-vue-next'
import { Button } from '@/shared/ui/button'
import type { MotorConfig, FeatureOption } from '../model/types'

const props = defineProps<{
  motor: MotorConfig
  expanded: boolean
}>()

const emit = defineEmits<{
  (e: 'toggle', id: number): void
  (e: 'reset', id: number): void
}>()

const featureOptions: FeatureOption[] = [
  { key: 'coolStep', label: 'CoolStep' },
  { key: 'spreadCycle', label: 'SpreadCycle' },
  { key: 'interpolation', label: 'Interpolation' },
  { key: 'pwmAutoScale', label: 'PWM Auto' },
]

// Helper for slider gradient
const getSliderBackground = (val: number, min: number, max: number) => {
  const percentage = ((val - min) / (max - min)) * 100
  return `linear-gradient(to right, var(--primary) 0%, var(--primary) ${percentage}%, var(--muted) ${percentage}%, var(--muted) 100%)`
}
</script>

<template>
  <div
    class="rounded-xl shadow-sm border-2 overflow-hidden transition-all duration-200 bg-card"
    :class="expanded ? 'border-primary ring-1 ring-primary/20' : 'border-border hover:border-primary/50'"
  >
    <!-- Header / Summary -->
    <div
      class="p-5 cursor-pointer transition-colors select-none"
      :class="expanded ? 'bg-primary/5' : 'bg-card hover:bg-muted/30'"
      @click="emit('toggle', motor.id)"
    >
      <div class="flex items-center justify-between">
        <div class="flex-1">
          <h3 class="text-lg font-bold flex items-center gap-2" :class="expanded ? 'text-primary' : 'text-foreground'">
            {{ motor.name }}
          </h3>

          <div class="flex items-center gap-4 mt-2 text-sm text-muted-foreground">
            <span class="flex items-center gap-1.5">
              <Zap class="w-3.5 h-3.5" />
              {{ motor.runCurrent }}mA
            </span>
            <span class="flex items-center gap-1.5">
              <ZapOff class="w-3.5 h-3.5" />
              {{ motor.holdCurrent }}mA
            </span>
            <span class="flex items-center gap-1.5">
              <Activity class="w-3.5 h-3.5" />
              1/{{ motor.microsteps }}
            </span>
          </div>

          <!-- Badges -->
          <div class="flex items-center gap-2 mt-3 flex-wrap">
            <template v-for="feature in featureOptions" :key="feature.key">
              <span
                v-if="motor[feature.key]"
                class="px-2 py-0.5 rounded text-[10px] font-semibold uppercase tracking-wide border"
                :class="expanded
                  ? 'bg-primary text-primary-foreground border-primary'
                  : 'bg-secondary text-secondary-foreground border-transparent'"
              >
                {{ feature.label }}
              </span>
            </template>
            <span
              v-if="!motor.coolStep && !motor.spreadCycle && !motor.interpolation && !motor.pwmAutoScale"
              class="text-xs text-muted-foreground/60 italic"
            >
              Standard Mode
            </span>
          </div>
        </div>

        <!-- Enable Toggle -->
        <div class="flex items-center gap-3 ml-4" @click.stop>
          <label class="flex items-center gap-2 cursor-pointer">
            <input
              type="checkbox"
              v-model="motor.enabled"
              class="w-5 h-5 rounded border-input text-primary focus:ring-primary"
            />
            <span class="text-sm font-medium" :class="motor.enabled ? 'text-foreground' : 'text-muted-foreground'">
              {{ motor.enabled ? 'ON' : 'OFF' }}
            </span>
          </label>
        </div>
      </div>
    </div>

    <!-- Expanded Details -->
    <div v-if="expanded" class="p-6 space-y-6 border-t bg-card/50">
      <div class="flex justify-end">
        <Button variant="outline" size="sm" class="h-8" @click="emit('reset', motor.id)">
          <RotateCcw class="w-3.5 h-3.5 mr-2" />
          Reset Defaults
        </Button>
      </div>

      <!-- Run Current -->
      <div>
        <div class="flex items-center gap-2 mb-3 text-sm font-medium text-foreground">
          <Zap class="w-4 h-4 text-primary" /> Run Current
        </div>
        <div class="flex items-center gap-4">
          <input
            type="range"
            min="200"
            max="2000"
            step="50"
            v-model.number="motor.runCurrent"
            class="flex-1 h-2 rounded-lg appearance-none cursor-pointer bg-muted"
            :style="{ background: getSliderBackground(motor.runCurrent, 200, 2000) }"
          />
          <div class="px-3 py-1.5 rounded-md font-mono text-sm font-bold bg-muted min-w-[80px] text-center">
            {{ motor.runCurrent }} mA
          </div>
        </div>
      </div>

      <!-- Hold Current -->
      <div>
        <div class="flex items-center gap-2 mb-3 text-sm font-medium text-foreground">
          <ZapOff class="w-4 h-4 text-primary" /> Hold Current
        </div>
        <div class="flex items-center gap-4">
          <input
            type="range"
            min="100"
            max="1500"
            step="50"
            v-model.number="motor.holdCurrent"
            class="flex-1 h-2 rounded-lg appearance-none cursor-pointer bg-muted"
            :style="{ background: getSliderBackground(motor.holdCurrent, 100, 1500) }"
          />
          <div class="px-3 py-1.5 rounded-md font-mono text-sm font-bold bg-muted min-w-[80px] text-center">
            {{ motor.holdCurrent }} mA
          </div>
        </div>
      </div>

      <!-- Microsteps -->
      <div>
        <div class="flex items-center gap-2 mb-3 text-sm font-medium text-foreground">
          <Activity class="w-4 h-4 text-primary" /> Microstepping
        </div>
        <select
          v-model.number="motor.microsteps"
          class="w-full px-3 py-2 rounded-md border bg-background text-sm focus:outline-none focus:ring-2 focus:ring-primary"
        >
          <option :value="1">Full Step (1/1)</option>
          <option :value="2">Half Step (1/2)</option>
          <option :value="4">Quarter Step (1/4)</option>
          <option :value="8">1/8 Step</option>
          <option :value="16">1/16 Step</option>
          <option :value="32">1/32 Step</option>
          <option :value="64">1/64 Step</option>
          <option :value="256">1/256 Step (Max)</option>
        </select>
      </div>

      <!-- StallGuard -->
      <div>
        <div class="flex items-center gap-2 mb-3 text-sm font-medium text-foreground">
          <Gauge class="w-4 h-4 text-primary" /> StallGuard Threshold
        </div>
        <div class="flex items-center gap-4">
          <input
            type="range"
            min="-64"
            max="63"
            v-model.number="motor.stallThreshold"
            class="flex-1 h-2 rounded-lg appearance-none cursor-pointer bg-muted"
            :style="{ background: getSliderBackground(motor.stallThreshold, -64, 63) }"
          />
          <div class="px-3 py-1.5 rounded-md font-mono text-sm font-bold bg-muted min-w-[80px] text-center">
            {{ motor.stallThreshold }}
          </div>
        </div>
      </div>

      <!-- Advanced Features -->
      <div>
        <div class="flex items-center gap-2 mb-3 text-sm font-medium text-foreground">
          <Cpu class="w-4 h-4 text-primary" /> Advanced Features
        </div>
        <div class="grid grid-cols-2 gap-3">
          <label
            v-for="feature in featureOptions"
            :key="feature.key"
            class="flex items-center gap-3 p-3 rounded-lg border cursor-pointer transition-all hover:bg-muted/50"
            :class="motor[feature.key] ? 'border-primary bg-primary/5' : 'border-border'"
          >
            <input
              type="checkbox"
              v-model="motor[feature.key]"
              class="w-4 h-4 rounded border-input text-primary focus:ring-primary"
            />
            <span class="font-medium text-sm">{{ feature.label }}</span>
          </label>
        </div>
      </div>
    </div>
  </div>
</template>