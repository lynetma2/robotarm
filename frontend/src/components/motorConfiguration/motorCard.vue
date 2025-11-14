<script setup lang="ts">
import {ref} from "vue";
import { Button } from "@/components/ui/button"
import {
  Card,
  CardContent,
  CardDescription,
  CardFooter,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import {
  Tabs,
  TabsContent,
  TabsList,
  TabsTrigger,
} from "@/components/ui/tabs"
import Tmc2209settings from "@/components/motorConfiguration/tmc2209settings.vue";

const modelValue = ref([50])
</script>

<template>
  <Card class="w-[350px]">
    <CardHeader>
      <CardTitle>Motor Configuration</CardTitle>
      <CardDescription>This changes the motor with id = 0</CardDescription>
    </CardHeader>
    <CardContent>
     <tmc2209settings/>
    </CardContent>
  </Card>
</template>

<style scoped>

</style>
