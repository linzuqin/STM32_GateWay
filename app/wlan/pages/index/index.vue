<template>
	<view class="host">
		<!-- 整页横滑容器：手势左右滑动切换三个面板 -->
		<swiper
			class="page-swiper"
			:current="current"
			:duration="300"
			:indicator-dots="false"
			@change="onPageChange"
		>
			<swiper-item>
				<scroll-view scroll-y class="panel-scroll" :show-scrollbar="false">
					<home-panel @go-settings="goSettings" />
				</scroll-view>
			</swiper-item>
			<swiper-item>
				<scroll-view scroll-y class="panel-scroll" :show-scrollbar="false">
					<settings-panel />
				</scroll-view>
			</swiper-item>
			<swiper-item>
				<scroll-view scroll-y class="panel-scroll" :show-scrollbar="false">
					<device-panel />
				</scroll-view>
			</swiper-item>
			<swiper-item>
				<scroll-view scroll-y class="panel-scroll" :show-scrollbar="false">
					<msh-panel />
				</scroll-view>
			</swiper-item>
			<swiper-item>
				<scroll-view scroll-y class="panel-scroll" :show-scrollbar="false">
					<account-panel />
				</scroll-view>
			</swiper-item>
		</swiper>

		<custom-tabbar :current="current" @change="onTabChange" />
	</view>
</template>

<script>
	export default {
		data() {
			return {
				current: 0,
				titles: ['OneNet 设备监控', '风扇设置', '设备详情', 'MSH 指令', '账户']
			}
		},
		methods: {
			switchTo(i) {
				if (i === this.current) return
				this.current = i
				uni.setNavigationBarTitle({ title: this.titles[i] })
			},
			// 点击底部菜单
			onTabChange(i) {
				this.switchTo(i)
			},
			// 手势横滑页面
			onPageChange(e) {
				this.switchTo(e.detail.current)
			},
			goSettings() {
				this.switchTo(1)
			}
		}
	}
</script>

<style>
	/* 四边定位撑满导航栏以下区域，避免 100vh 在各端高度不一致 */
	.host {
		position: absolute;
		top: 0;
		left: 0;
		right: 0;
		bottom: 0;
		display: flex;
		flex-direction: column;
		background: #f4f6fa;
	}

	.page-swiper {
		flex: 1;
		min-height: 0;
		width: 100%;
	}

	.panel-scroll {
		height: 100%;
	}

	/* 内容底部留出浮动菜单空间 */
	.panel-scroll .page {
		padding-bottom: 190rpx;
	}
</style>
